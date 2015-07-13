void ComboBox::create(Window &parent, unsigned x, unsigned y, unsigned width, unsigned height, const string &text) {
  comboBox->setParent(parent.window->container);
  comboBox->setGeometry(x, y, width, height);

  if(*text) {
    lstring list;
    list.split("\n", text);
    foreach(item, list) addItem(item);
  }

  comboBox->connect(comboBox, SIGNAL(currentIndexChanged(int)), SLOT(onChange()));
  if(parent.window->defaultFont) comboBox->setFont(*parent.window->defaultFont);
  comboBox->show();
}

void ComboBox::reset() {
  while(comboBox->count()) comboBox->removeItem(0);
}

void ComboBox::addItem(const string &text) {
  comboBox->addItem(QString::fromUtf8(text));
}

unsigned ComboBox::selection() {
  signed index = comboBox->currentIndex();
  return (index >= 0 ? index : 0);
}

void ComboBox::setSelection(unsigned row) {
  object->locked = true;
  comboBox->setCurrentIndex(row);
  object->locked = false;
}

ComboBox::ComboBox() {
  comboBox = new ComboBox::Data(*this);
  widget->widget = comboBox;
}
