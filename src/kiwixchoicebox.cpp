#include "kiwixchoicebox.h"
#include "ui_kiwixchoicebox.h"
#include "klistwidgetitem.h"
#include "kiwixapp.h"
#include "choiceitem.h"
#include <QFile>
#include "choiceitem.h"
#include "kiwixapp.h"
#include <QFile>
#include <QDebug>
#include <QAbstractItemView>
#include <QScrollBar>
#include "kiwixlineedit.h"

KiwixChoiceBox::KiwixChoiceBox(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::kiwixchoicebox)
{
    m_defaultItem = nullptr;
    ui->setupUi(this);
    ui->clearButton->setText(gt("clear"));
    ui->clearButton->setToolTip(gt("clear-filter"));

    choiceLabel = ui->choiceLabel;
    choiceLabel->setText(gt("undefined"));

    choiceSelector = new QListWidget(parent);
    choiceSelector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    choiceSelector->setMaximumWidth(250);
    choiceSelector->setMaximumHeight(200);
    choiceSelector->setCursor(Qt::PointingHandCursor);
    choiceSelector->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
    choiceSelector->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    choiceSelector->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    choiceSelector->setSelectionMode(QAbstractItemView::SelectionMode::MultiSelection);

    currentChoicesLayout = new FlowLayout(ui->currentChoices, 2, 2, 2);
    searcher = new KiwixLineEdit();
    searcher->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    currentChoicesLayout->addWidget(searcher);
    connect(choiceSelector, &QListWidget::itemPressed, this, [=](QListWidgetItem *item) {
        searcher->clear();
        if (item->isSelected()) {
            addSelection(item->text(), item->data(Qt::UserRole).toString());
        } else {
            removeSelection(item->text());
        }
    });

    connect(searcher, &QLineEdit::textChanged, [=](QString search) {
        searcher->setStyleSheet("QLineEdit{color: #666;}");
        int visibleItems = 0;
        for (auto i = 0; i < choiceSelector->count(); i++) {
            auto itemAtRow = choiceSelector->item(i);
            itemAtRow->setHidden(!itemAtRow->text().contains(search, Qt::CaseSensitivity::CaseInsensitive));
            visibleItems += !(itemAtRow->isHidden());
        }
        choiceSelector->setGeometry(this->x() + ui->currentChoices->x(),
                                    this->y() + ui->currentChoices->y() + ui->currentChoices->height(),
                                    choiceSelector->width(),
                                    visibleItems * KListWidgetItem::getItemHeight() + 2);
        choiceSelector->setVisible(true);
    });

    connect(searcher, &KiwixLineEdit::clicked, [=]() {
        choiceSelector->setVisible(true);
        adjustSize();
    });

    choiceSelector->setVisible(false);
    searcher->setStyleSheet("QLineEdit{color: #999;}");

    connect(searcher, &KiwixLineEdit::focusedOut, [=]() {
        searcher->setPlaceholderText(gt("search-" + m_type.toLower()));
        searcher->setStyleSheet("QLineEdit{color: #999;}");
        choiceSelector->setVisible(false);
        ui->currentChoices->setStyleSheet("#currentChoices{border: 1px solid #ccc;}");
    });

    connect(searcher, &KiwixLineEdit::focusedIn, [=]() {
        ui->currentChoices->setStyleSheet("#currentChoices{border: 2px solid #4e63ad;}");
        adjustSize();
        choiceSelector->setVisible(true);
        choiceSelector->raise();
        searcher->setPlaceholderText("");
    });

    ui->clearButton->setCursor(Qt::PointingHandCursor);
    connect(ui->clearButton, &QPushButton::clicked, [=]() {
       clearSelections();
       emit(choiceUpdated(getCurrentSelected()));
    });

    connect(this, &KiwixChoiceBox::choiceUpdated, [=]() {
        choiceSelector->setVisible(false);
    });
}

KiwixChoiceBox::~KiwixChoiceBox()
{
    delete ui;
}

void KiwixChoiceBox::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        searcher->clearFocus();
    }
}

bool KiwixChoiceBox::addSelection(QString key, QString value)
{
    auto chItem = new ChoiceItem(key, value);
    connect(chItem, &ChoiceItem::closeButtonClicked, [=](QString text) {
        removeSelection(text);
    });
    chItem->setObjectName(key);
    currentChoicesLayout->insertWidget(ui->currentChoices->children().count() - 2, chItem);
    searcher->setFixedWidth(20);
    searcher->setFocus();
    emit(choiceUpdated(getCurrentSelected()));
    return true;
}

bool KiwixChoiceBox::removeSelection(QString selection)
{
    auto chItem = ui->currentChoices->findChild<ChoiceItem*>(selection);
    chItem->deleteLater();
    emit(choiceUpdated(getCurrentSelected()));
    return true;
}

void KiwixChoiceBox::clearSelections()
{
    for (auto &item : choiceSelector->selectedItems()) {
        item->setSelected(false);
        auto chItem  = ui->currentChoices->findChild<ChoiceItem*>(item->text());
        ui->currentChoices->layout()->removeWidget(chItem);
        delete chItem;
    }
}

QString beautifyString(QString word)
{
    word = word.replace("_", " ");
    word[0] = word[0].toUpper();
    return word;
}

void KiwixChoiceBox::setSelections(QStringList selections, QString defaultSelection)
{
    SelectionList sList;
    for (const auto &sel : selections) {
        sList.append({sel, sel});
    }
    setSelections(sList, defaultSelection);
}

void KiwixChoiceBox::setSelections(SelectionList selections, QString defaultSelection)
{
    choiceSelector->clear();
    for (const auto &selection: selections)
    {
        auto item = new KListWidgetItem(beautifyString(selection.second));
        item->setData(Qt::UserRole, selection.first);
        choiceSelector->addItem(item);
        if (selection.second ==  defaultSelection)
        {
            m_defaultItem = item;
        }
    }
    if (choiceSelector->selectedItems().isEmpty() && m_defaultItem) {
        m_defaultItem->setSelected(true);
        addSelection(m_defaultItem->text(), m_defaultItem->data(Qt::UserRole).toString());
    }
    adjustSize();
}

void KiwixChoiceBox::adjustSize()
{
    QWidget::adjustSize();
    choiceSelector->adjustSize();
    choiceSelector->setGeometry(this->x() + ui->currentChoices->x(),
                                this->y() + ui->currentChoices->y() + ui->currentChoices->height(),
                                choiceSelector->width(),
                                choiceSelector->count() * KListWidgetItem::getItemHeight() + 2); // 2 is for the border so that all elements are visible
}

void KiwixChoiceBox::setType(QString type)
{
    ui->choiceLabel->setText(type);
    m_type = type;
    searcher->setPlaceholderText(gt("search-" + type.toLower()));
}

QStringList KiwixChoiceBox::getCurrentSelected()
{
    if (choiceSelector->selectedItems().isEmpty())
        return {"all"};
    QStringList selections;
    for (auto &item : choiceSelector->selectedItems()) {
        selections.append(item->data(Qt::UserRole).toString());
    }
    return selections;
}
