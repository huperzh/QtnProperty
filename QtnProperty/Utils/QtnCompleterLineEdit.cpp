#include "QtnCompleterLineEdit.h"

#include <QCompleter>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QListView>
#include <QScrollBar>

enum
{
	POPUP_MARGIN = 6
};

class QtnCompleterLineEdit::ListView : public QListView
{
	unsigned disableHide;

public:
	ListView();

	void beginDisableHide();
	void endDisableHide();
	virtual void setVisible(bool) override;
};

class QtnCompleterLineEdit::Completer : public QCompleter
{
	QtnCompleterLineEdit *mLineEdit;
	ListView *mListView;
	bool mCompleting;

public:
	explicit Completer(QtnCompleterLineEdit *lineEdit);
	virtual ~Completer() override;

	virtual bool eventFilter(QObject *watched, QEvent *event) override;

	void complete();
};

QtnCompleterLineEdit::QtnCompleterLineEdit(QWidget *parent)
	: QLineEdit(parent)
{
	setCompleter(new Completer(this));
}

QAbstractItemModel *QtnCompleterLineEdit::completerModel() const
{
	return completer()->model();
}

void QtnCompleterLineEdit::setCompleterModel(QAbstractItemModel *model)
{
	completer()->setModel(model);
}

void QtnCompleterLineEdit::complete()
{
	Q_ASSERT(dynamic_cast<Completer *>(this->completer()));
	auto completer = static_cast<Completer *>(this->completer());
	auto unselectedPrefix = text();
	if (selectionLength() > 0)
		unselectedPrefix.resize(selectionStart());
	if (unselectedPrefix != completer->completionPrefix())
		completer->setCompletionPrefix(unselectedPrefix);
	completer->complete();
}

bool QtnCompleterLineEdit::event(QEvent *e)
{
	return QLineEdit::event(e);
}

QtnCompleterLineEdit::ListView::ListView()
	: disableHide(0)
{
	setUniformItemSizes(true);
	setLayoutMode(Batched);
	setEditTriggers(NoEditTriggers);
	setResizeMode(Adjust);
	setTextElideMode(Qt::ElideNone);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setSelectionBehavior(SelectRows);
	setSelectionMode(SingleSelection);
}

inline void QtnCompleterLineEdit::ListView::beginDisableHide()
{
	disableHide++;
}

inline void QtnCompleterLineEdit::ListView::endDisableHide()
{
	disableHide--;
}

void QtnCompleterLineEdit::ListView::setVisible(bool yes)
{
	if (yes || !disableHide)
		QListView::setVisible(yes);
}

QtnCompleterLineEdit::Completer::Completer(QtnCompleterLineEdit *lineEdit)
	: QCompleter(lineEdit)
	, mLineEdit(nullptr) //do not remove or eventFilter will crash
	, mListView(new ListView)
	, mCompleting(false)
{
	setCompletionMode(QCompleter::PopupCompletion);
	setFilterMode(Qt::MatchContains);
	setWrapAround(true);
	setCaseSensitivity(Qt::CaseInsensitive);
	setPopup(mListView);

	mListView->viewport()->installEventFilter(this);

	mLineEdit = lineEdit;
}

QtnCompleterLineEdit::Completer::~Completer()
{
	mListView->viewport()->removeEventFilter(this);
}

bool QtnCompleterLineEdit::Completer::eventFilter(
	QObject *watched, QEvent *event)
{
	if (!mLineEdit)
		return QCompleter::eventFilter(watched, event);

	bool shouldComplete = false;
	bool disableHide = false;
	bool acceptEvent = false;
	bool escapePressed = false;
	bool finishEdit = false;
	if (!mLineEdit->isReadOnly())
	{
		switch (event->type())
		{
			case QEvent::FocusIn:
				shouldComplete = watched == mLineEdit;
				break;

			case QEvent::MouseButtonPress:
			case QEvent::MouseMove:
			case QEvent::MouseButtonRelease:
			case QEvent::MouseButtonDblClick:
				if (mLineEdit->rect().contains(mLineEdit->mapFromGlobal(
						static_cast<QMouseEvent *>(event)->globalPos())))
				{
					shouldComplete = !mListView->isVisible();
					disableHide = true;
					if (watched != mLineEdit)
					{
						mLineEdit->event(event);
					}
					break;
				}
				if (watched != mLineEdit)
				{
					if (event->type() == QEvent::MouseButtonRelease ||
						(!(mListView->isVisible() &&
							 mListView->rect().contains(
								 mListView->mapFromGlobal(
									 static_cast<QMouseEvent *>(event)
										 ->globalPos()))) &&
							event->type() == QEvent::MouseButtonPress))
					{
						acceptEvent = true;
						finishEdit = true;
					}
				}
				break;

			case QEvent::KeyPress:
			{
				auto ke = static_cast<QKeyEvent *>(event);
				switch (ke->key())
				{
					case Qt::Key_Up:
					case Qt::Key_Down:
						break;

					case Qt::Key_Left:
					case Qt::Key_Right:
					case Qt::Key_PageDown:
					case Qt::Key_PageUp:
					case Qt::Key_Home:
					case Qt::Key_End:
					case Qt::Key_Tab:
					case Qt::Key_Backtab:
						acceptEvent = true;
						break;

					case Qt::Key_Enter:
					case Qt::Key_Return:
						break;

					case Qt::Key_Escape:
						escapePressed = true;
						if (watched == mLineEdit ||
							!mListView->selectionModel()->hasSelection())
						{
							break;
						}
						// fallthrough
					default:
					{
						acceptEvent = true;
						disableHide = true;
						shouldComplete = true;
						break;
					}
				}
				break;
			}

			default:
				break;
		}
	}
	if (disableHide)
		mListView->beginDisableHide();

	bool result = QCompleter::eventFilter(watched, event);
	if (acceptEvent && !result && watched != mLineEdit && event->isAccepted())
	{
		watched->event(event);
		result = true;
	}

	if (disableHide)
		mListView->endDisableHide();

	if (finishEdit)
	{
		auto index = currentIndex();
		if (index.isValid())
			emit activated(index);
		emit mLineEdit->editingFinished();
	} else if (escapePressed)
	{
		if (watched != mLineEdit && mListView->selectionModel()->hasSelection())
		{
			mListView->clearSelection();
		} else
		{
			emit mLineEdit->escaped();
		}
		result = true;
	} else if (shouldComplete)
	{
		mLineEdit->complete();
	}

	return result;
}

void QtnCompleterLineEdit::Completer::complete()
{
	if (mCompleting)
		return;

	mCompleting = true;

	auto popup = this->popup();
	QRect rect(0, 0,
		qMax(mLineEdit->width(), popup->sizeHintForColumn(0) + POPUP_MARGIN),
		mLineEdit->height());

	int h = popup->sizeHintForRow(0) *
			qMin(maxVisibleItems(), completionModel()->rowCount()) +
		POPUP_MARGIN;
	popup->setMinimumHeight(h);
	QCompleter::complete(rect);

	QScrollBar *hsb = popup->horizontalScrollBar();
	if (hsb && hsb->isVisible())
		h += popup->horizontalScrollBar()->sizeHint().height();
	if (popup->height() < h)
	{
		popup->setMinimumHeight(h);

		QCompleter::complete(rect);
	}

	mCompleting = false;
}
