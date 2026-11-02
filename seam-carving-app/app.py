from __future__ import annotations

import os
import shutil
import struct
import subprocess
import tempfile
import threading
import warnings
from io import BytesIO
from pathlib import Path

from flask import Flask, Response, jsonify, render_template, request
from PIL import Image, ImageOps


APP_DIR = Path(__file__).resolve().parent
ENGINE_DIR = APP_DIR / "engine"
ENGINE_BINARY = APP_DIR / ".build" / "seam-carve"
MAX_UPLOAD_BYTES = 12 * 1024 * 1024
MAX_PIXELS = 1_000_000
MAX_DIMENSION = 4096
MAX_REMOVAL = 300
PROCESS_TIMEOUT_SECONDS = 120

Image.MAX_IMAGE_PIXELS = MAX_PIXELS

application = Flask(__name__, template_folder="templates", static_folder="static")
application.config["MAX_CONTENT_LENGTH"] = MAX_UPLOAD_BYTES + 64 * 1024
application.config["TEMPLATES_AUTO_RELOAD"] = True

_engine_lock = threading.Lock()
_processing_lock = threading.BoundedSemaphore(value=1)


class EngineUnavailable(RuntimeError):
    pass


def ensure_engine() -> Path:
    sources = [
        ENGINE_DIR / "runner.c",
        ENGINE_DIR / "c_img.c",
        ENGINE_DIR / "seamcarving.c",
    ]
    newest_source = max(path.stat().st_mtime_ns for path in sources)
    if ENGINE_BINARY.exists() and ENGINE_BINARY.stat().st_mtime_ns >= newest_source:
        return ENGINE_BINARY

    with _engine_lock:
        if ENGINE_BINARY.exists() and ENGINE_BINARY.stat().st_mtime_ns >= newest_source:
            return ENGINE_BINARY

        compiler = shutil.which(os.environ.get("CC", "cc"))
        if compiler is None:
            raise EngineUnavailable("No C compiler was found.")

        ENGINE_BINARY.parent.mkdir(parents=True, exist_ok=True)
        temporary_binary = ENGINE_BINARY.with_suffix(f".{os.getpid()}.tmp")
        command = [
            compiler,
            "-std=c11",
            "-O2",
            "-Wall",
            "-Wextra",
            str(ENGINE_DIR / "runner.c"),
            str(ENGINE_DIR / "c_img.c"),
            str(ENGINE_DIR / "seamcarving.c"),
            "-lm",
            "-o",
            str(temporary_binary),
        ]
        try:
            completed = subprocess.run(
                command,
                check=False,
                capture_output=True,
                text=True,
                timeout=30,
            )
        except (OSError, subprocess.TimeoutExpired) as exc:
            temporary_binary.unlink(missing_ok=True)
            raise EngineUnavailable("The C engine could not be compiled.") from exc

        if completed.returncode != 0:
            temporary_binary.unlink(missing_ok=True)
            application.logger.error("C engine compilation failed: %s", completed.stderr[-2000:])
            raise EngineUnavailable("The C engine could not be compiled.")

        temporary_binary.replace(ENGINE_BINARY)
        return ENGINE_BINARY


def error_response(message: str, status: int) -> tuple[Response, int]:
    return jsonify(error=message), status


@application.get("/")
def index() -> str:
    return render_template("index.html")


@application.post("/api/carve")
def carve() -> Response | tuple[Response, int]:
    upload = request.files.get("image")
    if upload is None or not upload.filename:
        return error_response("Choose a JPG or PNG image first.", 400)

    upload.stream.seek(0, os.SEEK_END)
    upload_size = upload.stream.tell()
    upload.stream.seek(0)
    if upload_size > MAX_UPLOAD_BYTES:
        return error_response("Image is over the 12 MB upload limit.", 413)

    try:
        target_width = int(request.form.get("target_width", ""))
    except ValueError:
        return error_response("Enter a whole number for the target width.", 400)
    if target_width < 1:
        return error_response("Target width must be at least 1 pixel.", 400)

    if not _processing_lock.acquire(blocking=False):
        return error_response("An image is already being processed. Try again shortly.", 429)

    try:
        with warnings.catch_warnings():
            warnings.simplefilter("error", Image.DecompressionBombWarning)
            try:
                with Image.open(upload.stream) as image:
                    if image.format not in {"JPEG", "PNG"}:
                        return error_response("Use a JPG or PNG image.", 415)
                    image.verify()

                upload.stream.seek(0)
                with Image.open(upload.stream) as image:
                    normalized = ImageOps.exif_transpose(image).convert("RGB")
            except (Image.DecompressionBombError, Image.DecompressionBombWarning):
                return error_response("This image is too large. Use an image under 1 million pixels.", 413)
            except (OSError, ValueError):
                return error_response("This image could not be opened. Try another JPG or PNG.", 400)

        width, height = normalized.size
        if width > MAX_DIMENSION or height > MAX_DIMENSION or width * height > MAX_PIXELS:
            return error_response("Use an image no larger than 1 million pixels or 4096 pixels on an edge.", 413)
        if target_width >= width:
            return error_response("Target width must be smaller than the original width.", 400)
        if width - target_width > MAX_REMOVAL:
            return error_response("Reduce the width by 300 pixels or less per edit.", 400)

        try:
            engine = ensure_engine()
        except (EngineUnavailable, OSError) as exc:
            application.logger.error("Seam carving engine unavailable: %s", exc)
            return error_response("The C engine could not start. Make sure a C compiler is installed.", 500)

        with tempfile.TemporaryDirectory(prefix="seam-carving-") as temporary_directory:
            temporary_path = Path(temporary_directory)
            input_path = temporary_path / "input.bin"
            output_path = temporary_path / "output.bin"
            input_path.write_bytes(struct.pack(">HH", height, width) + normalized.tobytes())

            try:
                completed = subprocess.run(
                    [str(engine), str(input_path), str(output_path), str(target_width)],
                    check=False,
                    capture_output=True,
                    text=True,
                    timeout=PROCESS_TIMEOUT_SECONDS,
                )
            except subprocess.TimeoutExpired:
                return error_response(
                    "This edit took too long. Try a smaller image or a smaller width reduction.", 504
                )
            except OSError:
                return error_response("The C engine could not start. Restart the app and try again.", 500)

            if completed.returncode != 0 or not output_path.exists():
                application.logger.error("Seam carving failed: %s", completed.stderr[-2000:])
                return error_response("The image could not be resized. Try a smaller width reduction.", 500)

            binary_result = output_path.read_bytes()
            expected_bytes = 4 + 3 * height * target_width
            if len(binary_result) != expected_bytes:
                application.logger.error("C engine returned an invalid output length.")
                return error_response("The image could not be resized. Try again with another image.", 500)

        result_image = Image.frombytes("RGB", (target_width, height), binary_result[4:])
        png_buffer = BytesIO()
        result_image.save(png_buffer, format="PNG", optimize=False)

        response = Response(png_buffer.getvalue(), mimetype="image/png")
        response.headers["Cache-Control"] = "no-store"
        response.headers["X-Original-Width"] = str(width)
        response.headers["X-Result-Width"] = str(target_width)
        response.headers["X-Image-Height"] = str(height)
        return response
    finally:
        _processing_lock.release()


@application.errorhandler(413)
def request_too_large(_error: Exception) -> tuple[Response, int]:
    return error_response("Image is over the 12 MB upload limit.", 413)


if __name__ == "__main__":
    application.run(host="127.0.0.1", port=int(os.environ.get("PORT", "5000")), debug=False, threaded=True)
