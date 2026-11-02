const form = document.querySelector('#carveForm')
const imageInput = document.querySelector('#imageInput')
const dropZone = document.querySelector('#dropZone')
const targetWidthInput = document.querySelector('#targetWidth')
const widthNote = document.querySelector('#widthNote')
const runButton = document.querySelector('#runButton')
const statusMessage = document.querySelector('#statusMessage')
const originalPreview = document.querySelector('#originalPreview')
const resultPreview = document.querySelector('#resultPreview')
const originalEmpty = document.querySelector('#originalEmpty')
const resultEmpty = document.querySelector('#resultEmpty')
const originalDimensions = document.querySelector('#originalDimensions')
const resultDimensions = document.querySelector('#resultDimensions')
const downloadButton = document.querySelector('#downloadButton')

const maxFileBytes = 12 * 1024 * 1024
const maxSeams = 300
let selectedFile = null
let selectedWidth = 0
let imageObjectUrl = null
let resultObjectUrl = null
let isProcessing = false

function getTargetWidth() {
  const width = targetWidthInput.valueAsNumber
  return Number.isInteger(width) ? width : Number.NaN
}

function setStatus(message, isError = false) {
  statusMessage.textContent = message
  statusMessage.classList.toggle('is-error', isError)
}

function clearResult() {
  if (resultObjectUrl) URL.revokeObjectURL(resultObjectUrl)
  resultObjectUrl = null
  resultPreview.removeAttribute('src')
  resultPreview.hidden = true
  resultEmpty.hidden = false
  resultDimensions.textContent = ''
  downloadButton.removeAttribute('href')
  downloadButton.removeAttribute('download')
  downloadButton.setAttribute('aria-disabled', 'true')
  downloadButton.setAttribute('tabindex', '-1')
}

function updateRunButton() {
  const targetWidth = getTargetWidth()
  const reduction = selectedWidth - targetWidth
  runButton.disabled = isProcessing || !selectedFile || !Number.isInteger(targetWidth) || targetWidth < 1 || reduction < 1 || reduction > maxSeams
}

function useFile(file) {
  if (!file || isProcessing) return
  if (file.size > maxFileBytes) {
    setStatus(selectedFile ? 'That image is over 12 MB. The current preview is unchanged.' : 'Image is over the 12 MB upload limit.', true)
    return
  }
  if (file.type && !['image/jpeg', 'image/png'].includes(file.type)) {
    setStatus(selectedFile ? 'Use a JPG or PNG image. The current preview is unchanged.' : 'Use a JPG or PNG image.', true)
    return
  }

  selectedFile = file
  selectedWidth = 0
  if (imageObjectUrl) URL.revokeObjectURL(imageObjectUrl)
  clearResult()
  imageObjectUrl = URL.createObjectURL(file)
  originalPreview.src = imageObjectUrl
  originalPreview.hidden = false
  originalEmpty.hidden = true
  originalDimensions.textContent = 'Reading…'
  targetWidthInput.value = ''
  targetWidthInput.disabled = true
  widthNote.textContent = 'Reading image dimensions…'
  runButton.classList.remove('is-busy')
  setStatus('Loading image…')
  updateRunButton()
}

imageInput.addEventListener('change', () => useFile(imageInput.files?.[0]))

originalPreview.addEventListener('load', () => {
  if (!selectedFile) return
  selectedWidth = originalPreview.naturalWidth
  const height = originalPreview.naturalHeight
  const minimumWidth = Math.max(1, selectedWidth - maxSeams)
  const defaultReduction = Math.min(64, Math.max(1, Math.round(selectedWidth * 0.08)))
  const defaultWidth = Math.max(minimumWidth, selectedWidth - defaultReduction)
  targetWidthInput.min = String(minimumWidth)
  targetWidthInput.max = String(selectedWidth - 1)
  targetWidthInput.value = String(defaultWidth)
  targetWidthInput.disabled = selectedWidth <= 1
  originalDimensions.textContent = `${selectedWidth} × ${height}`
  widthNote.textContent = `Reduce up to ${selectedWidth - minimumWidth} px per edit.`
  if (selectedWidth <= 1) setStatus('This image is already too narrow to resize.', true)
  else setStatus('Ready to carve.')
  updateRunButton()
})

originalPreview.addEventListener('error', () => {
  originalPreview.hidden = true
  originalEmpty.hidden = false
  selectedFile = null
  selectedWidth = 0
  targetWidthInput.disabled = true
  widthNote.textContent = 'Choose a JPG or PNG image.'
  setStatus('This image could not be opened. Try another JPG or PNG.', true)
  updateRunButton()
})

targetWidthInput.addEventListener('input', () => {
  clearResult()
  if (selectedWidth) {
    const targetWidth = getTargetWidth()
    if (!Number.isInteger(targetWidth)) {
      setStatus('Enter a whole number for the target width.', true)
    } else if (targetWidth < 1 || targetWidth >= selectedWidth) {
      setStatus('Target width must be smaller than the original width.', true)
    } else if (selectedWidth - targetWidth > maxSeams) {
      setStatus(`Reduce the width by ${maxSeams} pixels or less per edit.`, true)
    } else {
      setStatus('Ready to carve.')
    }
  }
  updateRunButton()
})

dropZone.addEventListener('keydown', (event) => {
  if (event.key === 'Enter' || event.key === ' ') {
    event.preventDefault()
    imageInput.click()
  }
})

for (const eventName of ['dragenter', 'dragover']) {
  dropZone.addEventListener(eventName, (event) => {
    event.preventDefault()
    dropZone.classList.add('is-dragging')
  })
}
for (const eventName of ['dragleave', 'drop']) {
  dropZone.addEventListener(eventName, (event) => {
    event.preventDefault()
    dropZone.classList.remove('is-dragging')
  })
}
dropZone.addEventListener('drop', (event) => useFile(event.dataTransfer?.files?.[0]))

form.addEventListener('submit', async (event) => {
  event.preventDefault()
  if (!selectedFile || isProcessing) return

  const targetWidth = getTargetWidth()
  const reduction = selectedWidth - targetWidth
  if (!Number.isInteger(targetWidth)) {
    setStatus('Enter a whole number for the target width.', true)
    return
  }
  if (targetWidth < 1 || reduction < 1) {
    setStatus('Target width must be between 1 pixel and the original width minus 1 pixel.', true)
    return
  }
  if (reduction > maxSeams) {
    setStatus('Choose a target width up to 300 pixels narrower.', true)
    return
  }

  const body = new FormData()
  body.append('image', selectedFile, selectedFile.name)
  body.append('target_width', String(targetWidth))
  isProcessing = true
  updateRunButton()
  imageInput.disabled = true
  dropZone.setAttribute('aria-disabled', 'true')
  runButton.classList.add('is-busy')
  runButton.querySelector('span').textContent = 'Carving seams…'
  setStatus('Carving seams. Larger images can take a moment.')

  try {
    const response = await fetch('/api/carve', { method: 'POST', body })
    if (!response.ok) {
      const problem = await response.json().catch(() => ({}))
      throw new Error(problem.error || 'The image could not be resized. Try again.')
    }

    const blob = await response.blob()
    resultObjectUrl = URL.createObjectURL(blob)
    resultPreview.src = resultObjectUrl
    resultPreview.hidden = false
    resultEmpty.hidden = true

    const resultWidth = Number(response.headers.get('X-Result-Width')) || targetWidth
    const height = Number(response.headers.get('X-Image-Height')) || originalPreview.naturalHeight
    resultDimensions.textContent = `${resultWidth} × ${height}`

    const baseName = selectedFile.name.replace(/\.[^.]+$/, '').replace(/[^a-z0-9_-]+/gi, '-') || 'image'
    downloadButton.href = resultObjectUrl
    downloadButton.download = `${baseName}-carved-${resultWidth}px.png`
    downloadButton.setAttribute('aria-disabled', 'false')
    downloadButton.removeAttribute('tabindex')
    setStatus('Finished. Compare the result or download the PNG.')
  } catch (error) {
    setStatus(error instanceof Error ? error.message : 'The image could not be resized. Try again.', true)
  } finally {
    isProcessing = false
    imageInput.disabled = false
    dropZone.removeAttribute('aria-disabled')
    runButton.classList.remove('is-busy')
    runButton.querySelector('span').textContent = 'Run seam carving'
    updateRunButton()
  }
})

window.addEventListener('beforeunload', () => {
  if (imageObjectUrl) URL.revokeObjectURL(imageObjectUrl)
  if (resultObjectUrl) URL.revokeObjectURL(resultObjectUrl)
})
