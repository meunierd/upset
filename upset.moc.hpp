class UpsetWindow : public QWidget {
  Q_OBJECT

public:
  UpsetWindow();

private slots:
  void selectPatchFile();
  void selectTargetFile();
  void applyPatch();

private:
  QGridLayout *layout;
  QLabel *patchLabel;
  QLineEdit *patchFilename;
  QPushButton *patchSelect;
  QLabel *targetLabel;
  QLineEdit *targetFilename;
  QPushButton *targetSelect;
  QProgressBar *progressBar;
  QPushButton *applyButton;

  file patchFile;
  file inputFile;
  file outputFile;
  unsigned outputTargetSize;
  unsigned progress;

  uint32_t patchChecksum;
  uint32_t inputChecksum;
  uint32_t outputChecksum;

  unsigned decode();
  uint8_t patchRead();
  uint8_t inputRead();
  void outputWrite(uint8_t data);
  void updateProgressBar();
};
