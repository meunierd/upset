#define QT_CORE_LIB
#include <QApplication>
#include <QtGui>

#include <nall/algorithm.hpp>
#include <nall/crc32.hpp>
#include <nall/detect.hpp>
#include <nall/file.hpp>
#include <nall/platform.hpp>
#include <nall/stdint.hpp>
#include <nall/string.hpp>
#include <nall/utility.hpp>
using namespace nall;

#include "upset.moc.hpp"
#include "upset.moc"

UpsetWindow *upsetWindow;

UpsetWindow::UpsetWindow() {
  setWindowTitle("Upset v01");
  resize(480, 0);

  layout = new QGridLayout;
  layout->setAlignment(Qt::AlignTop);
  layout->setMargin(5);
  layout->setSpacing(5);
  setLayout(layout);

  patchLabel = new QLabel("Patch File:");
  layout->addWidget(patchLabel, 0, 0);

  patchFilename = new QLineEdit;
  patchFilename->setReadOnly(true);
  layout->addWidget(patchFilename, 0, 1);

  patchSelect = new QPushButton(" Select ");
  layout->addWidget(patchSelect, 0, 2);

  targetLabel = new QLabel("Target File:");
  layout->addWidget(targetLabel, 1, 0);

  targetFilename = new QLineEdit;
  targetFilename->setReadOnly(true);
  layout->addWidget(targetFilename, 1, 1);

  targetSelect = new QPushButton(" Select ");
  layout->addWidget(targetSelect, 1, 2);

  progressBar = new QProgressBar;
  progressBar->setValue(0);
  progressBar->setVisible(false);
  layout->addWidget(progressBar, 2, 0, 1, 2);

  applyButton = new QPushButton(" Apply Patch ");
  applyButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  applyButton->setEnabled(false);
  layout->addWidget(applyButton, 2, 2);

  #if defined(PLATFORM_WIN)
  move(64, 64);
  #endif
  show();

  connect(patchSelect, SIGNAL(released()), SLOT(selectPatchFile()));
  connect(targetSelect, SIGNAL(released()), SLOT(selectTargetFile()));
  connect(applyButton, SIGNAL(released()), SLOT(applyPatch()));
}

void UpsetWindow::selectPatchFile() {
  QString filename = QFileDialog::getOpenFileName(this, "Select Patch File", "", "UPS Patches (*.ups)");
  patchFilename->setText(filename);
  if(patchFilename->text() != "" && patchFilename->text() == targetFilename->text()) {
    QMessageBox::warning(this, "Upset", "<b>Warning:</b><br>Patch filename cannot be the same as the target filename.");
    targetFilename->setText("");
  }
  applyButton->setEnabled(patchFilename->text() != "" && targetFilename->text() != "");
}

void UpsetWindow::selectTargetFile() {
  QString filename = QFileDialog::getOpenFileName(this, "Select Target File", "", "All Files (*)");
  targetFilename->setText(filename);
  if(patchFilename->text() != "" && patchFilename->text() == targetFilename->text()) {
    QMessageBox::warning(this, "Upset", "<b>Warning:</b><br>Patch filename cannot be the same as the target filename.");
    targetFilename->setText("");
  }
  applyButton->setEnabled(patchFilename->text() != "" && targetFilename->text() != "");
}

void UpsetWindow::applyPatch() {
  string patchName = patchFilename->text().toUtf8().constData();
  string inputName = targetFilename->text().toUtf8().constData();
  string outputName = sprint(inputName, ".tmp");
  string backupName = sprint(inputName, ".bak");

  try {
    if(patchFile.open(patchName, file::mode_read) == false) {
      throw "Error opening patch file for reading.";
    }

    if(inputFile.open(inputName, file::mode_readwrite) == false) {
      throw "Error opening target file for reading.";
    }

    if(patchFile.size() < 18) throw "Patch file is invalid or corrupt.";

    patchChecksum = ~0;
    inputChecksum = ~0;
    outputChecksum = ~0;

    if(patchRead() != 'U') throw "Patch file is invalid or corrupt.";
    if(patchRead() != 'P') throw "Patch file is invalid or corrupt.";
    if(patchRead() != 'S') throw "Patch file is invalid or corrupt.";
    if(patchRead() != '1') throw "Patch file is invalid or corrupt.";

    unsigned fxSize = decode();
    unsigned fySize = decode();
    if(inputFile.size() != fxSize && inputFile.size() != fySize) throw "Input file does not match the file specified in the patch.";
    outputTargetSize = (inputFile.size() == fxSize ? fySize : fxSize);

    patchFilename->setEnabled(false);
    patchSelect->setEnabled(false);
    targetFilename->setEnabled(false);
    targetSelect->setEnabled(false);
    progressBar->setVisible(true);
    applyButton->setEnabled(false);

    if(outputFile.open(outputName, file::mode_write) == false) {
      throw "Error opening target file for writing.";
    }

    progress = 0;
    unsigned relative = 0;
    while(patchFile.offset() < patchFile.size() - 12) {
      unsigned offset = relative;
      relative += decode();
      while(offset++ < relative) outputWrite(inputRead());
      while(true) {
        uint8_t x = inputRead();
        uint8_t y = patchRead();
        outputWrite(x ^ y);
        if(y == 0) break;
      }
    }

    inputChecksum = ~inputChecksum;
    outputChecksum = ~outputChecksum;

    uint32_t fxChecksum = 0, fyChecksum = 0;
    for(unsigned i = 0; i < 4; i++) fxChecksum |= patchRead() << (i << 3);
    for(unsigned i = 0; i < 4; i++) fyChecksum |= patchRead() << (i << 3);

    if((inputChecksum != fxChecksum || inputFile.size() != fxSize)
    && (inputChecksum != fyChecksum || inputFile.size() != fySize)) {
      throw "Input file does not match the file specified in the patch.";
    }

    if((outputChecksum != fxChecksum || outputTargetSize != fxSize)
    && (outputChecksum != fyChecksum || outputTargetSize != fySize)) {
      throw "Output file does not match the file specified in the patch.";
    }

    uint32_t checksum = ~patchChecksum, fzChecksum = 0;
    for(unsigned i = 0; i < 4; i++) fzChecksum |= patchRead() << (i << 3);
    if(checksum != fzChecksum) throw "Patch file is corrupt.";

    patchFile.close();
    inputFile.close();
    outputFile.close();

    QMessageBox::information(this, "Upset", "Patching was successful!");

    if(QFile::exists(backupName)) QFile::remove(backupName);
    QFile::rename(inputName, backupName);
    QFile::rename(outputName, inputName);

    QApplication::quit();
  } catch(const char *error) {
    if(patchFile.open()) patchFile.close();
    if(inputFile.open()) inputFile.close();
    if(outputFile.open()) outputFile.close();
    if(QFile::exists(outputName)) QFile::remove(outputName);

    QMessageBox::critical(this, "Upset", sprint("<b>Error:</b><br>", error));

    patchFilename->setEnabled(true);
    patchSelect->setEnabled(true);
    targetFilename->setEnabled(true);
    targetSelect->setEnabled(true);
    progressBar->setVisible(false);
    applyButton->setEnabled(true);
  }
}

unsigned UpsetWindow::decode() {
  unsigned offset = 0, shift = 1;
  while(true) {
    uint8_t x = patchRead();
    offset += (x & 0x7f) * shift;
    if(x & 0x80) break;
    shift <<= 7;
    offset += shift;
  }
  return offset;
}

uint8_t UpsetWindow::patchRead() {
  if(patchFile.offset() >= patchFile.size()) throw "Patch file is corrupt.";
  uint8_t data = patchFile.read();
  patchChecksum = crc32_adjust(patchChecksum, data);
  return data;
}

uint8_t UpsetWindow::inputRead() {
  if(inputFile.offset() >= inputFile.size()) return 0x00;
  uint8_t data = inputFile.read();
  inputChecksum = crc32_adjust(inputChecksum, data);
  return data;
}

void UpsetWindow::outputWrite(uint8_t data) {
  updateProgressBar();
  if(outputFile.offset() >= outputTargetSize) return;
  outputChecksum = crc32_adjust(outputChecksum, data);
  outputFile.write(data);
}

void UpsetWindow::updateProgressBar() {
  unsigned inputProgress = 100 * inputFile.offset() / inputFile.size();
  unsigned outputProgress = 100 * outputFile.offset() / outputTargetSize;
  unsigned newProgress = min(inputProgress, outputProgress);

  if(newProgress != progress) {
    progress = newProgress;
    progressBar->setValue(progress);
    QApplication::processEvents();
  }
}

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  upsetWindow = new UpsetWindow;
  return app.exec();
}
