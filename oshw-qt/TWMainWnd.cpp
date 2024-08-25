/* Copyright (C) 2001-2017 by Madhav Shanbhag and Eric Schmidt
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include "TWMainWnd.h"
#include "TWApp.h"

#include "../generic/generic.h"

#include "../gen.h"
#include "../defs.h"
#include "../messages.h"
#include "../settings.h"
#include "../score.h"
#include "../state.h"
#include "../play.h"
#include "../oshw.h"
#include "../err.h"
#include "../help.h"
#include "TWTextCoder.h"

extern int pedanticmode;

#include <QApplication>
#include <QClipboard>

#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QShortcut>

#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>

#include <QTextDocument>

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

#include <QPainter>
#include <QPalette>
#include <QBrush>
#include <QLinearGradient>

#include <QDir>
#include <QFileInfo>

#include <QString>
#include <QTextStream>

#include <vector>

#include <cstring>
#include <cctype>

class TWStyledItemDelegate : public QStyledItemDelegate
{
public:
	TWStyledItemDelegate(QObject* pParent = nullptr)
		: QStyledItemDelegate(pParent) {}
		
	void paint(QPainter* pPainter, const QStyleOptionViewItem& option,
		const QModelIndex& index) const override;
};


void TWStyledItemDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& _option,
	const QModelIndex& index) const
{
	QStyleOptionViewItem option = _option;
	option.state &= ~QStyle::State_HasFocus;
	QStyledItemDelegate::paint(pPainter, option, index);
}

// ... All this just to remove a silly little dotted focus rectangle


class TWTableModel : public QAbstractTableModel
{
public:
	TWTableModel(QObject* pParent = nullptr);
	void SetTableSpec(const tablespec* pSpec);
	
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	
protected:
	struct ItemInfo
	{
		QString sText;
		Qt::Alignment align;
		ItemInfo() : align(Qt::AlignCenter) {}
	};
	
	int m_nRows, m_nCols;
	std::vector<ItemInfo> m_vecItems;
	
	QVariant GetData(int row, int col, int role) const;
};


TWTableModel::TWTableModel(QObject* pParent)
	:
	QAbstractTableModel(pParent),
	m_nRows(0), m_nCols(0)
{
}

void TWTableModel::SetTableSpec(const tablespec* pSpec)
{
	beginResetModel();

	m_nRows = pSpec->rows;
	m_nCols = pSpec->cols;
	int n = m_nRows * m_nCols;
	
	m_vecItems.clear();
	m_vecItems.reserve(n);
	
	ItemInfo dummyItemInfo;
	
	const char* const * pp = pSpec->items;
	for (int i = 0; i < n; ++pp)
	{
		const char* p = *pp;
		ItemInfo ii;

		ii.sText = TWTextCoder::decode(p + 2);
		// The "center dot" character (U+00B7) isn't very visible in
		// some fonts, so we use U+25CF instead.
		ii.sText.replace(QChar(0x00B7), QChar(0x25CF));

		char c = p[1];
		Qt::Alignment ha = (c=='+' ? Qt::AlignRight : c=='.' ? Qt::AlignHCenter : Qt::AlignLeft);
		ii.align = (ha | Qt::AlignVCenter);
		
		m_vecItems.push_back(ii);

		int d = p[0] - '0';
		for (int j = 1; j < d; ++j)
		{
			m_vecItems.push_back(dummyItemInfo);
		}

		i += d;
	}
	
	endResetModel();
}

int TWTableModel::rowCount(const QModelIndex& parent) const
{
	return m_nRows-1;
}

int TWTableModel::columnCount(const QModelIndex& parent) const
{
	return m_nCols;
}

QVariant TWTableModel::GetData(int row, int col, int role) const
{
	int i = row*m_nCols + col;
	const ItemInfo& ii = m_vecItems[i];
	
	switch (role)
	{
		case Qt::DisplayRole:
			return ii.sText;
			
		case Qt::TextAlignmentRole:
			return int(ii.align);
		
		default:
			return QVariant();
	}
}

QVariant TWTableModel::data(const QModelIndex& index, int role) const
{
	return GetData(1+index.row(), index.column(), role);
}

QVariant TWTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
		return GetData(0, section, role);
	else
		return QVariant();
}


TileWorldMainWnd::TileWorldMainWnd(QWidget* pParent, Qt::WindowFlags flags)
	:
	QMainWindow(pParent, flags/*|Qt::FramelessWindowHint*/),
	m_bSetupUi(false),
	m_bWindowClosed(false),
	m_pSurface(),
	m_pInvSurface(),
	m_nKeyState(),
	m_shortMessages(),
	m_bKbdRepeatEnabled(true),
	m_nRuleset(Ruleset_None),
	m_nLevelNum(0),
	m_bProblematic(false),
	m_bOFNT(false),
	m_nBestTime(TIME_NIL),
	m_hintMode(HINT_EMPTY),
	m_nTimeLeft(TIME_NIL),
	m_bTimedLevel(false),
	m_bReplay(false),
    m_title(""),
    m_author(""),
	m_pSortFilterProxyModel()
{
	setupUi(this);
	m_bSetupUi = true;
	
	QLayout* pGameLayout = m_pGamePage->layout();
	if (pGameLayout != nullptr)
	{
		pGameLayout->setAlignment(m_pGameFrame, Qt::AlignCenter);
		pGameLayout->setAlignment(m_pInfoFrame, Qt::AlignCenter);
		pGameLayout->setAlignment(m_pObjectsFrame, Qt::AlignCenter);
		pGameLayout->setAlignment(m_pMessagesFrame, Qt::AlignHCenter);
	}
	
	QPalette pal = m_pMainWidget->palette();
	QLinearGradient gradient(0, 0, 1, 1);
	gradient.setCoordinateMode(QGradient::StretchToDeviceMode);
	QColor color = pal.window().color();
	gradient.setColorAt(0, color.lighter(125));
	gradient.setColorAt(1, color.darker(125));
	pal.setBrush(QPalette::Window, QBrush(gradient));
	m_pMainWidget->setPalette(pal);

	m_pTblList->setItemDelegate(new TWStyledItemDelegate(m_pTblList));
	
	m_pTextBrowser->setSearchPaths(QStringList{ QString::fromLocal8Bit(seriesdatdir) });
	
	g_pApp->installEventFilter(this);

	connect(m_pTblList, &QTableView::activated, this, &TileWorldMainWnd::OnListItemActivated);
	connect(m_pRadioMs, &QRadioButton::toggled, this, &TileWorldMainWnd::OnRulesetSwitched);
	connect(m_pTxtFind, &QLineEdit::textChanged, this, &TileWorldMainWnd::OnFindTextChanged);
	connect(m_pTxtFind, &QLineEdit::returnPressed, this, &TileWorldMainWnd::OnFindReturnPressed);
	connect(m_pBtnPlay, &QToolButton::clicked, this, &TileWorldMainWnd::OnPlayback);
	connect(m_pSldSpeed, &QSlider::valueChanged, this, &TileWorldMainWnd::OnSpeedValueChanged);
	connect(m_pSldSpeed, &QSlider::sliderReleased, this, &TileWorldMainWnd::OnSpeedSliderReleased);
	connect(m_pSldSeek, &QSlider::valueChanged, this, &TileWorldMainWnd::OnSeekPosChanged);
	connect(m_pBtnTextNext, &QToolButton::clicked, this, &TileWorldMainWnd::OnTextNext);
	connect(m_pBtnTextPrev, &QToolButton::clicked, this, &TileWorldMainWnd::OnTextPrev);
	connect(m_pBtnTextReturn, &QToolButton::clicked, this, &TileWorldMainWnd::OnTextReturn);

	connect(new QShortcut(Qt::Key_Escape, m_pTextPage), &QShortcut::activated, this, &TileWorldMainWnd::OnTextReturn);
	connect(new QShortcut(Qt::CTRL|Qt::Key_R, m_pTextPage), &QShortcut::activated, this, &TileWorldMainWnd::OnTextReturn);
	connect(new QShortcut(Qt::CTRL|Qt::Key_N, m_pTextPage), &QShortcut::activated, this, &TileWorldMainWnd::OnTextNext);
	connect(new QShortcut(Qt::CTRL|Qt::Key_P, m_pTextPage), &QShortcut::activated, this, &TileWorldMainWnd::OnTextPrev);
	connect(new QShortcut(Qt::Key_N, m_pTextPage), &QShortcut::activated, this, &TileWorldMainWnd::OnTextNext);
	connect(new QShortcut(Qt::Key_P, m_pTextPage), &QShortcut::activated, this, &TileWorldMainWnd::OnTextPrev);
	
	connect(m_pMenuBar, &QMenuBar::triggered, this, &TileWorldMainWnd::OnMenuActionTriggered);

	action_displayCCX->setChecked(getintsetting("displayccx"));
	action_forceShowTimer->setChecked(getintsetting("forceshowtimer") > 0);
	if (getintsetting("selectedruleset") == Ruleset_Lynx)
		m_pRadioLynx->setChecked(true);
	else
		m_pRadioMs->setChecked(true);

	int const tickMS = 1000 / TICKS_PER_SECOND;
	startTimer(tickMS / 2);
}


TileWorldMainWnd::~TileWorldMainWnd()
{
	g_pApp->removeEventFilter(this);

	TW_FreeSurface(m_pInvSurface);
	TW_FreeSurface(m_pSurface);
}


void TileWorldMainWnd::closeEvent(QCloseEvent* pCloseEvent)
{
	QMainWindow::closeEvent(pCloseEvent);
	m_bWindowClosed = true;

	if (m_pMainWidget->currentIndex() == PAGE_GAME)
		g_pApp->ExitTWorld();
	else
		g_pApp->quit();
}

void TileWorldMainWnd::timerEvent(QTimerEvent*)
{
	if (m_shortMessages.empty())
		return;

	uint32_t nCurTime = TW_GetTicks();
	QPalette::ColorRole style = QPalette::BrightText;
	bool switchMessage = false;
	while (!m_shortMessages.empty())
	{
		auto & mData = m_shortMessages.back();
		if (nCurTime <= mData.nMsgUntil)
		{
			if (nCurTime > mData.nMsgBoldUntil)
				style = QPalette::Text;
			break;
		}
		else
		{
			m_shortMessages.pop_back();
			switchMessage = true;
		}
	}
	if (switchMessage)
	{
		if (m_shortMessages.empty())
			m_pLblShortMsg->clear();
		else
		{
			m_pLblShortMsg->setText(m_shortMessages.back().sMsg);
		}
	}
	if (style != m_pLblShortMsg->foregroundRole())
		m_pLblShortMsg->setForegroundRole(style);
}

bool TileWorldMainWnd::eventFilter(QObject* pObject, QEvent* pEvent)
{
	if (HandleEvent(pObject, pEvent))
		return true;
	return QMainWindow::eventFilter(pObject, pEvent);
}

struct QtModifier_TWKey
{
	Qt::KeyboardModifier nQtMod;
	int nTWKey;
};

static const QtModifier_TWKey g_modKeys[] =
{
	{Qt::ShiftModifier,		TWK_LSHIFT},
	{Qt::ControlModifier,	TWK_LCTRL},
	{Qt::AltModifier,		TWK_LALT},
	{Qt::MetaModifier,		TWK_LMETA}
};

bool TileWorldMainWnd::HandleEvent(QObject* pObject, QEvent* pEvent)
{
	if (!m_bSetupUi) return false;
	
	QEvent::Type eType = pEvent->type();
	// if (eType == QEvent::LayoutRequest) puts("QEvent::LayoutRequest");

	switch (eType)
	{
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
		{
			QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);

			int nQtKey = pKeyEvent->key();
			if (nQtKey == Qt::Key_Backtab)
				nQtKey = Qt::Key_Tab;

			int nTWKey = -1;
			if (nQtKey >= 0  &&  nQtKey <= 0xFF)
				nTWKey = tolower(nQtKey);
			else if (nQtKey >= 0x01000000  &&  nQtKey <= 0x01000060)
				nTWKey = TWK_FUDGE(nQtKey);
			else
				return false;

			// Completely ignore multimedia keys, etc. and don't consume them
				
			bool bPress = (eType == QEvent::KeyPress);
			m_nKeyState[nTWKey] = bPress;
			// Always record the application key state

			// Handle modifier keys falling out of sync due to some events never being received
			// E.g., Windows 7 never sends Alt key-up after Alt+Tab
			for (const QtModifier_TWKey& mod : g_modKeys)
			{
				if (mod.nTWKey == nTWKey)
					continue;
				bool bModPressed = ((pKeyEvent->modifiers() & mod.nQtMod) != 0);
				if (m_nKeyState[mod.nTWKey] != bModPressed)
				{
					m_nKeyState[mod.nTWKey] = bModPressed;
					keyeventcallback(mod.nTWKey, bModPressed);
					// printf("*** MOD 0x%X = %d\n", mod.nTWKey, int(bModPressed));
				}
			}
				
			bool bConsume = (m_pMainWidget->currentIndex() == PAGE_GAME) &&
							(QApplication::activeModalWidget() == nullptr);
			// Only consume keys for the game, not for the tables or the message boxes
			//  with the exception of a few keys for the table
			QObjectList const & tableWidgets = m_pTablePage->children();
			if (bPress && tableWidgets.contains(pObject) &&  pKeyEvent->modifiers() == Qt::NoModifier)
			{
				bConsume = true;
				int currentrow = m_pTblList->selectionModel()->currentIndex().row();
				int maxrow = m_pSortFilterProxyModel->rowCount()-1;
				if (nQtKey == Qt::Key_Home)
					m_pTblList->selectRow(0);
				else if (nQtKey == Qt::Key_End)
					m_pTblList->selectRow(maxrow);
				else if (nQtKey == Qt::Key_Up)
					m_pTblList->selectRow(currentrow - 1);
				else if (nQtKey == Qt::Key_Down)
					m_pTblList->selectRow(currentrow + 1);
				else if ((nQtKey == Qt::Key_Return || nQtKey == Qt::Key_Enter) && currentrow >= 0)
					g_pApp->exit(CmdProceed);
				else if (nQtKey == Qt::Key_Right && m_pRadioMs->isVisible() && pObject != m_pTxtFind)
					m_pRadioLynx->setChecked(true);
				else if (nQtKey == Qt::Key_Left  && m_pRadioMs->isVisible() && pObject != m_pTxtFind)
					m_pRadioMs->setChecked(true);
				else if (nQtKey == Qt::Key_Escape)
					g_pApp->exit(CmdQuitLevel);
				else
					bConsume = false;
			}

			if (m_bKbdRepeatEnabled || !pKeyEvent->isAutoRepeat())
			{
				// if (bConsume)
				// Always pass the key events so that the game's internal key state is in sync
				//  otherwise, Ctrl/Shift remain "pressed" after Ctrl+S / Shift+Tab / etc.
				{
					// printf("nTWKey=0x%03X bPress=%d bConsume=%d\n", nTWKey, int(bPress), int(bConsume));
					keyeventcallback(nTWKey, bPress);
				}
			}
			return bConsume;
		}
		break;
		
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
		{
			if (pObject != m_pGameWidget)
				return false;
			QMouseEvent* pMouseEvent = static_cast<QMouseEvent*>(pEvent);
			mouseeventcallback(pMouseEvent->pos().x(), pMouseEvent->pos().y(), pMouseEvent->button(),
				(eType == QEvent::MouseButtonPress));
			return true;
		}
		break;
		
		case QEvent::Wheel:
		{
			if (pObject != m_pGameWidget)
				return false;
			QWheelEvent* pWheelEvent = static_cast<QWheelEvent*>(pEvent);
			const int scrollDelta = pWheelEvent->angleDelta().y();
			if (scrollDelta != 0)
			{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
				const QPointF eventPos(pWheelEvent->position());
#else
				const QPoint eventPos(pWheelEvent->x(), pWheelEvent->y());
#endif
				mouseeventcallback(int(eventPos.x()), int(eventPos.y()),
					(scrollDelta > 0 ? TW_BUTTON_WHEELUP : TW_BUTTON_WHEELDOWN),
					true);
			}
			return true;
		}
		break;

		default:
			break;
	}
	
	return false;
}


extern "C" uint8_t* TW_GetKeyState(int* pnNumKeys)
{
	return g_pMainWnd->GetKeyState(pnNumKeys);
}

uint8_t* TileWorldMainWnd::GetKeyState(int* pnNumKeys)
{
	if (pnNumKeys != nullptr)
		*pnNumKeys = TWK_LAST;
	return m_nKeyState;
}


void TileWorldMainWnd::PulseKey(int nTWKey)
{
	keyeventcallback(nTWKey, true);
	keyeventcallback(nTWKey, false);
}


void TileWorldMainWnd::OnPlayback()
{
	int nTWKey = m_bReplay ? TWC_PAUSEGAME : TWC_PLAYBACK;
	PulseKey(nTWKey);
}

/*
 * Keyboard input functions.
 */
 
/* Turn keyboard repeat on or off. If enable is TRUE, the keys other
 * than the direction keys will repeat at the standard rate.
 */
int setkeyboardrepeat(int enable)
{
	if (g_pMainWnd) {
		return g_pMainWnd->SetKeyboardRepeat(enable);
	}
	else {
		return true;
	}
}

bool TileWorldMainWnd::SetKeyboardRepeat(bool bEnable)
{
	m_bKbdRepeatEnabled = bEnable;
	return true;
}


/*
 * Video output functions.
 */

/* Create a display surface appropriate to the requirements of the
 * game (e.g., sized according to the tiles and the font). FALSE is
 * returned on error.
 */
int creategamedisplay(void)
{
	return g_pMainWnd->CreateGameDisplay();
}

bool TileWorldMainWnd::CreateGameDisplay()
{
	TW_FreeSurface(m_pSurface);
	TW_FreeSurface(m_pInvSurface);
	
	int w = NXTILES*geng.wtile, h = NYTILES*geng.htile;
	m_pSurface = static_cast<Qt_Surface*>(TW_NewSurface(w, h, false));
	m_pInvSurface = static_cast<Qt_Surface*>(TW_NewSurface(4*geng.wtile, 2*geng.htile, false));

	m_pGameWidget->setPixmap(m_pSurface->GetPixmap());
	m_pObjectsWidget->setPixmap(m_pInvSurface->GetPixmap());

	m_pGameWidget->setFixedSize(m_pSurface->GetPixmap().size());
	m_pObjectsWidget->setFixedSize(m_pInvSurface->GetPixmap().size());

	geng.screen = m_pSurface;
	m_disploc = TW_Rect(0, 0, w, h);
	geng.maploc = m_pGameWidget->geometry();
	
	SetCurrentPage(PAGE_GAME);

	m_pControlsFrame->setVisible(true);
	// this->adjustSize();
	// this->resize(minimumSizeHint());
	// TODO: not working!

	return true;
}


void TileWorldMainWnd::SetCurrentPage(Page ePage)
{
	m_pMainWidget->setCurrentIndex(ePage);
	bool const showMenus = (ePage == PAGE_GAME);
	m_pMenuBar->setVisible(showMenus);

	// Menus won't disappear on some systems. Do the next best thing.
	m_pMenuBar->setEnabled(showMenus);
}



/* Select the colors used for drawing the display background, normal
 * text, bold (highlighted) text, and dim (grayed) text. The color
 * values are of the form 0x00RRGGBB.
 */
void setcolors(long bkgnd, long text, long bold, long dim)
{
	// N/A?
}


/* Fill the display with the background color.
 */
void cleardisplay(void)
{
	g_pMainWnd->ClearDisplay();
}

void TileWorldMainWnd::ClearDisplay()
{
	// TODO?
	geng.mapvieworigin = -1;
}


/* Display the current game state. timeleft and besttime provide the
 * current time on the clock and the best time recorded for the level,
 * measured in seconds.
 */
int displaygame(gamestate const *state, int timeleft, int besttime, int showinitstate)
{
	return g_pMainWnd->DisplayGame(state, timeleft, besttime, showinitstate);
}

bool TileWorldMainWnd::DisplayGame(const gamestate* pState, int nTimeLeft, int nBestTime, bool bShowInitState)
{
	bool const bInit = (pState->currenttime == -1);
	bool const bTimedLevel = (pState->game->time > 0);

	m_nTimeLeft = nTimeLeft;
	m_bTimedLevel = bTimedLevel;

	bool const bForceShowTimer = action_forceShowTimer->isChecked();

	if (bInit)
	{
		m_nRuleset = pState->ruleset;
		m_nLevelNum = pState->game->number;
		m_bProblematic = false;
		m_nBestTime = nBestTime;
		m_bReplay = false;	// IMPORTANT for OnSpeedValueChanged
		SetSpeed(0);	// IMPORTANT

		m_pGameWidget->setCursor(m_nRuleset==Ruleset_MS ? Qt::CrossCursor : Qt::ArrowCursor);

		m_pLCDNumber->display(pState->game->number);

		m_title = TWTextCoder::decode(pState->game->name);
        m_bOFNT = (m_title.compare(QStringLiteral("YOU CAN'T TEACH AN OLD FROG NEW TRICKS"),
                                  Qt::CaseInsensitive) == 0);

        m_pLblTitle->setText(m_title);
        Qt::AlignmentFlag halign = (m_pLblTitle->sizeHint().width() <= m_pLblTitle->width()) ? Qt::AlignHCenter : Qt::AlignLeft;
        m_pLblTitle->setAlignment(halign | Qt::AlignVCenter);

        m_author = TWTextCoder::decode(pState->game->author);
        if (m_author.isEmpty()) {
            m_author = m_ccxLevelset.vecLevels[m_nLevelNum].sAuthor;
        }
        if (!m_author.isEmpty()) {
            m_pLblAuthor->setText(m_author);
            halign = (m_pLblAuthor->sizeHint().width() <= m_pLblAuthor->width()) ? Qt::AlignHCenter : Qt::AlignLeft;
            m_pLblAuthor->setAlignment(halign | Qt::AlignVCenter);
            m_pLblAuthor->show();
        } else {
            m_pLblAuthor->hide();
        }
		
		m_pLblPassword->setText(TWTextCoder::decode(pState->game->passwd));

		m_pSldSeek->setValue(0);
		bool bHasSolution = hassolution(pState->game);
		m_pControlsFrame->setVisible(bHasSolution);
		
		menu_Game->setEnabled(true);
		menu_Solution->setEnabled(bHasSolution);
		menu_Help->setEnabled(true);
		action_GoTo->setEnabled(true);
		CCX::Level const & currLevel
		    (m_ccxLevelset.vecLevels[m_nLevelNum]);
		bool hasPrologue(!currLevel.txtPrologue.vecPages.empty());
		bool hasEpilogue(!currLevel.txtEpilogue.vecPages.empty());
		action_Prologue->setEnabled(hasPrologue);
		action_Epilogue->setEnabled(hasEpilogue && bHasSolution);

		m_pPrgTime->setPar(-1);
		
		bool bParBad = (pState->game->sgflags & SGF_REPLACEABLE) != 0;
		m_pPrgTime->setParBad(bParBad);
		QString a = bParBad ? QStringLiteral(" *") : QString();

		m_pPrgTime->setFullBar(!bTimedLevel);

		if (bTimedLevel)
		{
			if (nBestTime == TIME_NIL)
			{
				m_pPrgTime->setFormat(QStringLiteral("%v"));
			}
			else
			{
				m_pPrgTime->setFormat(QString::number(nBestTime) + a + QStringLiteral(" / %v"));
				m_pPrgTime->setPar(nBestTime);
				m_pSldSeek->setMaximum(nTimeLeft-nBestTime);
			}
			m_pPrgTime->setMaximum(pState->game->time);
			m_pPrgTime->setValue(nTimeLeft);
		}
		else
		{
			const QString noTime = (bForceShowTimer ? QStringLiteral("[999]") : QStringLiteral("---"));
			if (nBestTime == TIME_NIL)
				m_pPrgTime->setFormat(noTime);
			else
			{
				m_pPrgTime->setFormat(QLatin1Char('[') + QString::number(nBestTime) + a
					+ QStringLiteral("] / ") + noTime);
				m_pSldSeek->setMaximum(999-nBestTime);
			}
			m_pPrgTime->setMaximum(999);
			m_pPrgTime->setValue(999);
		}
		
		m_pLblHint->clear();
		SetHintMode(HINT_EMPTY);
		
		CheckForProblems(pState);
		
		Narrate(&CCX::Level::txtPrologue);
	}
	else
	{
		m_bReplay = (pState->replay >= 0);
		m_pControlsFrame->setVisible(m_bReplay);
		if (m_bProblematic)
		{
			m_pLblHint->clear();
			SetHintMode(HINT_EMPTY);
			m_bProblematic = false;
		}

		menu_Game->setEnabled(false);
		menu_Solution->setEnabled(false);
		menu_Help->setEnabled(false);

        menu_Game->hide();
        menu_Solution->hide();
        menu_Help->hide();

        action_GoTo->setEnabled(false);
		action_Prologue->setEnabled(false);
		action_Epilogue->setEnabled(false);
	}

	if (pState->statusflags & SF_SHUTTERED)
	{
		DisplayShutter();
	}
	else
	{
		DisplayMapView(pState);
	}
	
	for (int i = 0; i < 4; ++i)
	{
		drawfulltileid(m_pInvSurface, i*geng.wtile, 0,
			(pState->keys[i] ? Key_Red+i : Empty));
		drawfulltileid(m_pInvSurface, i*geng.wtile, geng.htile,
			(pState->boots[i] ? Boots_Ice+i : Empty));
	}
	m_pObjectsWidget->setPixmap(m_pInvSurface->GetPixmap());

	m_pLCDChipsLeft->display(pState->chipsneeded);
	
	m_pPrgTime->setValue(nTimeLeft);

	if (!bInit)
	{
		QString sFormat;
		if (bTimedLevel)
			sFormat = QStringLiteral("%v");
		else if (bForceShowTimer)
			sFormat = QStringLiteral("[%v]");
		else
			sFormat = QStringLiteral("---");

		if ((bTimedLevel || bForceShowTimer) && nBestTime != TIME_NIL) {
            int value = nTimeLeft-nBestTime;
            if (value >= 0)
                sFormat += QStringLiteral(" (+%1)").arg(value);
            else
                sFormat += QStringLiteral(" (%1)").arg(value);
        }
		m_pPrgTime->setFormat(sFormat);
	}

	if (m_bReplay && !m_pSldSeek->isSliderDown())
	{
		m_pSldSeek->blockSignals(true);
		m_pSldSeek->setValue(pState->currenttime / TICKS_PER_SECOND);
		m_pSldSeek->blockSignals(false);
	}

	if (!m_bProblematic)
	{
		// Call setText / clear only when really required
		// See comments about QLabel in TWDisplayWidget.h
		bool bShowHint = (pState->statusflags & SF_SHOWHINT) != 0;
		if (bShowInitState && m_bReplay)
		{
			if (SetHintMode(HINT_INITSTATE))
				m_pLblHint->setText(TWTextCoder::decode(getinitstatestring()));
		}
		else if (bShowHint)
		{
			if (SetHintMode(HINT_TEXT))
				m_pLblHint->setText(TWTextCoder::decode(pState->hinttext));
		}
		else if (SetHintMode(HINT_EMPTY))
			m_pLblHint->clear();
	}

	return true;
}

void TileWorldMainWnd::CheckForProblems(const gamestate* pState)
{
	QString s;

	if (pState->statusflags & SF_INVALID)
	{
		s = tr("This level cannot be played.");
	}
	else if (pState->game->unsolvable)
	{
		s = tr("This level is reported to be unsolvable");
		if (*pState->game->unsolvable)
			s += QStringLiteral(": ") + TWTextCoder::decode(pState->game->unsolvable);
		s += QLatin1Char('.');
	}
	else
	{
		CCX::RulesetCompatibility ruleCompat = m_ccxLevelset.vecLevels[m_nLevelNum].ruleCompat;
		CCX::Compatibility compat = CCX::COMPAT_UNKNOWN;
		if (m_nRuleset == Ruleset_Lynx)
		{
			if (pedanticmode)
				compat = ruleCompat.ePedantic;
			else
				compat = ruleCompat.eLynx;
		}
		else if (m_nRuleset == Ruleset_MS)
		{
			compat = ruleCompat.eMS;
		}
		if (compat == CCX::COMPAT_NO)
		{
			s = tr("This level is flagged as being incompatible with the current ruleset.");
		}
	}

	m_bProblematic = !s.isEmpty();
	if (m_bProblematic)
	{
		m_pLblHint->setText(s);
	}
}

void TileWorldMainWnd::DisplayMapView(const gamestate* pState)
{
	short xviewpos = pState->xviewpos;
	short yviewpos = pState->yviewpos;
	bool bFrogShow = (m_bOFNT  &&  m_bReplay  &&
	                  xviewpos/8 == 14  &&  yviewpos/8 == 9);
	if (bFrogShow)
	{
		int x = xviewpos, y = yviewpos;
		if (m_nRuleset == Ruleset_MS)
		{
			for (int pos = 0; pos < CXGRID*CYGRID; ++pos)
			{
				int id = pState->map[pos].top.id;
				if ( ! (id >= Teeth && id < Teeth+4) )
					continue;
				x = (pos % CXGRID) * 8;
				y = (pos / CXGRID) * 8;
				break;
			}
		}
		else
		{
			for (const creature* p = pState->creatures; p->id != 0; ++p)
			{
				if ( ! (p->id >= Teeth && p->id < Teeth+4) )
					continue;
				x = (p->pos % CXGRID) * 8;
				y = (p->pos / CXGRID) * 8;
				if (p->moving > 0)
				{
					switch (p->dir)
					{
					  case NORTH:	y += p->moving;	break;
					  case WEST:	x += p->moving;	break;
					  case SOUTH:	y -= p->moving;	break;
					  case EAST:	x -= p->moving;	break;
					}
				}
				break;
			}
		}
		const_cast<gamestate*>(pState)->xviewpos = x;
		const_cast<gamestate*>(pState)->yviewpos = y;
	}

	displaymapview(pState, m_disploc);
	m_pGameWidget->setPixmap(m_pSurface->GetPixmap());
	
	if (bFrogShow)
	{
		const_cast<gamestate*>(pState)->xviewpos = xviewpos;
		const_cast<gamestate*>(pState)->yviewpos = yviewpos;
	}
}

void TileWorldMainWnd::DisplayShutter()
{
	QPixmap pixmap(m_pGameWidget->size());
	pixmap.fill(Qt::black);

	QPainter painter(&pixmap);
	painter.setPen(Qt::red);
	QFont font;
	font.setPixelSize(geng.htile);
	painter.setFont(font);
	painter.drawText(pixmap.rect(), Qt::AlignCenter, tr("Paused"));
	painter.end();

	m_pGameWidget->setPixmap(pixmap);
}


void TileWorldMainWnd::OnSpeedValueChanged(int nValue)
{
	// IMPORTANT!
	if (!m_bReplay) return;
	// Even though the replay controls are hidden when play begins,
	//  the slider could be manipulated before making the first move
	
	SetSpeed(nValue);
}
	
void TileWorldMainWnd::SetSpeed(int nValue)
{
	int nMS = (m_nRuleset == Ruleset_MS) ? 1100 : 1000;
	if (nValue >= 0)
		settimersecond(nMS >> nValue);
	else
		settimersecond(nMS << (-nValue/2));
}

void TileWorldMainWnd::OnSpeedSliderReleased()
{
	m_pSldSpeed->setValue(0);
}


/* Get number of seconds to skip at start of playback.
 */
int getreplaysecondstoskip(void)
{
	return g_pMainWnd->GetReplaySecondsToSkip();
}

int TileWorldMainWnd::GetReplaySecondsToSkip() const
{
	return m_pSldSeek->value();
}


void TileWorldMainWnd::OnSeekPosChanged(int nValue)
{
	PulseKey(TWC_SEEK);
}


/* Display a short message appropriate to the end of a level's game
 * play. If the level was completed successfully, completed is TRUE,
 * and the other three arguments define the base score and time bonus
 * for the level, and the user's total score for the series; these
 * scores will be displayed to the user.
 */
int displayendmessage(int basescore, int timescore, long totalscore,
			     int completed)
{
	return g_pMainWnd->DisplayEndMessage(basescore, timescore, totalscore, completed);
}

void TileWorldMainWnd::ReleaseAllKeys()
{
	// On X11, it seems that the last key-up event (for the arrow key that resulted in completion)
	//  is never sent (neither to the main widget, nor to the message box).
	// So pretend that all keys being held down were released.
	for (int k = 0; k < TWK_LAST; ++k)
	{
		if (m_nKeyState[k])
		{
			m_nKeyState[k] = false;
			keyeventcallback(k, false);
			// printf("*** RESET 0x%X\n", k);
		}
	}
} 

int TileWorldMainWnd::DisplayEndMessage(int nBaseScore, int nTimeScore, long lTotalScore, int nCompleted)
{
	if (nCompleted == 0)
		return CmdNone;
		
	if (nCompleted == -2)	// abandoned
		return CmdNone;
		
	QMessageBox msgBox(this);
	
	if (nCompleted > 0)	// Success
	{
		const char* szMsg = nullptr;
		if (m_bReplay)
			szMsg = "Alright!";
		else
		{
			szMsg = getmessage(MessageWin);
			if (!szMsg)
				szMsg = "You won!";
		}

		QString sText;
		QTextStream strm(&sText);
		strm.setLocale(m_locale);
		strm
			<< "<table>"
			// << "<tr><td><hr></td></tr>"
			<< "<tr><td><big><b>" << m_title << "</b></big></td></tr>"
			// << "<tr><td><hr></td></tr>"
			;
		if (!m_author.isEmpty())
			strm << "<tr><td>by " << m_author << "</td></tr>";
		strm
			<< "<tr><td><hr></td></tr>"
			<< "<tr><td>&nbsp;</td></tr>"
			<< "<tr><td><big><b>" << szMsg << "</b></big>" << "</td></tr>"
			;

		if (!m_bReplay)
		{
		  if (m_bTimedLevel  &&  m_nBestTime != TIME_NIL)
		  {
			strm << "<tr><td>";
			if (m_nTimeLeft > m_nBestTime)
			{
				strm << "You made it " << (m_nTimeLeft - m_nBestTime) << " second(s) faster this time!";
			}
			else if (m_nTimeLeft == m_nBestTime)
			{
				strm << "You scored " << m_nBestTime << " yet again.";
			}
			else
			{
				strm << "But not as quick as your previous score of " << m_nBestTime << "...";
			}
			strm << "</td></tr>";
		  }

		  strm
			<< "<tr><td>&nbsp;</td></tr>"
			<< "<tr><td><table>"
			  << "<tr><td>Time Bonus:</td><td align='right'>"  << nTimeScore << "</td></tr>"
			  << "<tr><td>Level Bonus:</td><td align='right'>" << nBaseScore << "</td></tr>"
			  << "<tr><td>Level Score:</td><td align='right'>" << (nTimeScore + nBaseScore) << "</td></tr>"
			  << "<tr><td colspan='2'><hr></td></tr>"
			  << "<tr><td>Total Score:</td><td align='right'>" << lTotalScore << "</td></tr>"
			<< "</table></td></tr>"
			// << "<tr><td><pre>                                </pre></td></tr>"	// spacer
			<< "</table>"
			;
		}

		msgBox.setTextFormat(Qt::RichText);
		msgBox.setText(sText);

		Qt_Surface* pSurface = static_cast<Qt_Surface*>(TW_NewSurface(geng.wtile, geng.htile, false));
		drawfulltileid(pSurface, 0, 0, Exited_Chip);
		msgBox.setIconPixmap(pSurface->GetPixmap());
		TW_FreeSurface(pSurface);
		
		msgBox.setWindowTitle(m_bReplay ? tr("Replay Completed") : tr("Level Completed"));

		m_sTextToCopy = TWTextCoder::decode(
			timestring(m_nLevelNum,
                       TWTextCoder::encode(m_title).constData(),
				m_nTimeLeft, m_bTimedLevel, false));

		msgBox.addButton(tr("&Onward!"), QMessageBox::AcceptRole);
		QPushButton* pBtnRestart = msgBox.addButton(tr("&Restart"), QMessageBox::AcceptRole);
		QPushButton* pBtnCopyScore = msgBox.addButton(tr("&Copy Score"), QMessageBox::ActionRole);
		connect(pBtnCopyScore, &QPushButton::clicked, this, &TileWorldMainWnd::OnCopyText);
		
		msgBox.exec();
		ReleaseAllKeys();

		// macOS *does* return focus to the main window after closing the
		// victory dialog; this is here in case that changes
		this->activateWindow();
		if (msgBox.clickedButton() == pBtnRestart)
			return CmdSameLevel;
			
		// if (!m_bReplay)
			Narrate(&CCX::Level::txtEpilogue);
	}
	else	// Failure
	{
		bool bTimeout = (m_bTimedLevel  &&  m_nTimeLeft <= 0);
		if (m_bReplay)
		{
			QString sMsg = QStringLiteral("Whoa!  Chip ");
			if (bTimeout)
				sMsg += QStringLiteral("ran out of time");
			else
				sMsg += QStringLiteral("ran into some trouble");
			// TODO: What about when Chip just doesn't reach the exit or reaches the exit too early?
			sMsg += QStringLiteral(" there.\nIt looks like the level has changed after that solution"
				" was recorded.");
			msgBox.setText(sMsg);
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setWindowTitle(tr("Replay Failed"));
		}
		else
		{
			const char* szMsg = nullptr;
			if (bTimeout)
			{
				szMsg = getmessage(MessageTime);
				if (!szMsg)
					szMsg = "You ran out of time.";
			}
			else
			{
				szMsg = getmessage(MessageDie);
				if (!szMsg)
					szMsg = "You died.";
			}
			
			msgBox.setTextFormat(Qt::PlainText);
			msgBox.setText(TWTextCoder::decode(szMsg));
			// setIcon also causes the corresponding system sound to play
			// setIconPixmap does not
			QStyle* pStyle = QApplication::style();
			if (pStyle)
			{
				QIcon icon = pStyle->standardIcon(QStyle::SP_MessageBoxWarning);
				msgBox.setIconPixmap(icon.pixmap(48));
			}
			msgBox.setWindowTitle(tr("Oops."));
		}
		msgBox.exec();
		ReleaseAllKeys();
		// macOS doesn't return focus to the main window after closing the death
		// dialog; this fixes that
		this->activateWindow();
	}
	
	return CmdProceed;
}


/* Display a (very short) message for the given number of
 * milliseconds. bold indicates the number of milliseconds the
 * message is with highlighting. After that (if the message is
 * still visible) it is rendered as normal text.
 */
int setdisplaymsg(char const *msg, int msecs, int bold)
{
	return g_pMainWnd->SetDisplayMsg(msg, msecs, bold);
}

bool TileWorldMainWnd::SetDisplayMsg(const char* szMsg, int nMSecs, int nBoldMSecs)
{
	if (szMsg == nullptr || *szMsg == '\0')
	{
		m_pLblShortMsg->clear();
		m_shortMessages.clear();
	}

	uint32_t nCurTime = TW_GetTicks();
	uint32_t msgUntil = nCurTime + nMSecs;
	uint32_t boldUntil = nCurTime + nBoldMSecs;
	const QString sMsg = TWTextCoder::decode(szMsg);

	m_pLblShortMsg->setForegroundRole(QPalette::BrightText);
	m_pLblShortMsg->setText(sMsg);

	m_shortMessages.push_back({sMsg, msgUntil, boldUntil});
	return true;
}


/* Display a scrollable table. title provides a title to display. The
 * table's first row provides a set of column headers which will not
 * scroll. index points to the index of the item to be initially
 * selected; upon return, the value will hold the current selection.
 * Either listtype or inputcallback must be used to tailor the UI.
 * listtype specifies the type of list being displayed.
 * inputcallback points to a function that is called to retrieve
 * input. The function is passed a pointer to an integer. If the
 * callback returns TRUE, this integer should be set to either a new
 * index value or one of the following enum values. This value will
 * then cause the selection to be changed, whereupon the display will
 * be updated before the callback is called again. If the callback
 * returns FALSE, the table is removed from the display, and the value
 * stored in the integer will become displaylist()'s return value.
 */
int displaylist(char const *title, tablespec const *table, int *index,
		       DisplayListType listtype, int (*inputcallback)(int*))
{
	return g_pMainWnd->DisplayList(title, table, index, listtype, inputcallback);
}

int TileWorldMainWnd::DisplayList(const char* szTitle, const tablespec* pTableSpec, int* pnIndex,
		DisplayListType eListType, int (*pfnInputCallback)(int*))
{
  int nCmd = 0;
  
  // dummy scope to force model destructors before ExitTWorld
  {
	TWTableModel model;
	model.SetTableSpec(pTableSpec);
	QSortFilterProxyModel proxyModel;
	m_pSortFilterProxyModel = &proxyModel;
	proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
	proxyModel.setFilterKeyColumn(-1);
	proxyModel.setSourceModel(&model);
	m_pTblList->setModel(&proxyModel);
	
	QModelIndex index = proxyModel.mapFromSource(model.index(*pnIndex, 0));
	m_pTblList->setCurrentIndex(index);
	m_pTblList->resizeColumnsToContents();
	m_pTblList->resizeRowsToContents();
	m_pTxtFind->clear();
	SetCurrentPage(PAGE_TABLE);
	m_pTblList->setFocus();

	bool const showRulesetOptions = (eListType == LIST_MAPFILES);
	m_pRadioMs->setVisible(showRulesetOptions);
	m_pRadioLynx->setVisible(showRulesetOptions);
	m_pLblSpace->setVisible(showRulesetOptions);

	nCmd = g_pApp->exec();

	*pnIndex = proxyModel.mapToSource(m_pTblList->currentIndex()).row();

	SetCurrentPage(PAGE_GAME);
	m_pTblList->setModel(nullptr);
	m_pSortFilterProxyModel = nullptr;
  }
	
  if (m_bWindowClosed) g_pApp->ExitTWorld();

  return nCmd;
}

void TileWorldMainWnd::OnListItemActivated(const QModelIndex& index)
{
	g_pApp->exit(CmdProceed);
}

void TileWorldMainWnd::OnFindTextChanged(const QString& sText)
{
	if (!m_pSortFilterProxyModel) return;
	
	QString sWildcard = QStringLiteral("*");
	if (!sText.isEmpty())
		sWildcard += sText + QLatin1Char('*');
	m_pSortFilterProxyModel->setFilterWildcard(sWildcard);
}

void TileWorldMainWnd::OnFindReturnPressed()
{
	if (!m_pSortFilterProxyModel) return;

	int n = m_pSortFilterProxyModel->rowCount();
	if (n == 0)
	{
		ding();
		return;
	}

	m_pTblList->setFocus();

	if (!m_pTblList->currentIndex().isValid())
		m_pTblList->selectRow(0);

	if (n == 1)
		g_pApp->exit(CmdProceed);
}

void TileWorldMainWnd::OnRulesetSwitched(bool mschecked) {
	setintsetting("selectedruleset", mschecked ? Ruleset_MS : Ruleset_Lynx);
}

/* Display an input prompt to the user. prompt supplies the prompt to
 * display, and input points to a buffer to hold the user's input.
 * maxlen sets a maximum length to the input that will be accepted.
 * Either inputtype or inputcallback must be used to validate input.
 * inputtype indicates the type of input desired.
 * The supplied callback function is called repeatedly to obtain
 * input. If the callback function returns a printable ASCII
 * character, the function will automatically append it to the string
 * stored in input. If '\b' is returned, the function will erase the
 * last character in input, if any. If '\f' is returned the function
 * will set input to "". If '\n' is returned, the input prompt is
 * erased and displayinputprompt() returns TRUE. If a negative value
 * is returned, the input prompt is erased and displayinputprompt()
 * returns FALSE. All other return values from the callback are
 * ignored.
 */
int displayinputprompt(char const *prompt, char *input, int maxlen,
			      InputPromptType inputtype, int (*inputcallback)(void))
{
	return g_pMainWnd->DisplayInputPrompt(prompt, input, maxlen, inputtype, inputcallback);
}

int TileWorldMainWnd::DisplayInputPrompt(const char* szPrompt, char* pInput, int nMaxLen,
		InputPromptType eInputType, int (*pfnInputCallback)())
{
	switch (eInputType)
	{
		case INPUT_YESNO:
		{
			QMessageBox::StandardButton eBtn = QMessageBox::question(
				this, TileWorldApp::s_sTitle, TWTextCoder::decode(szPrompt),
				QMessageBox::Yes|QMessageBox::No);
			pInput[0] = (eBtn==QMessageBox::Yes) ? 'Y' : 'N';
			pInput[1] = '\0';
			return true;
		}
		
		case INPUT_ALPHA:
		default:
		{
			// TODO: proper validation, maybe embedded prompt
			QString sText = QInputDialog::getText(this, TileWorldApp::s_sTitle,
				TWTextCoder::decode(szPrompt));
			if (sText.isEmpty())
				return false;
			sText.truncate(nMaxLen);
			if (eInputType == INPUT_ALPHA)
				sText = sText.toUpper();
			strcpy(pInput, TWTextCoder::encode(sText).constData());
			return true;
		}
	}
}

/*
 * Miscellaneous functions.
 */

/* Ring the bell.
 */
void ding(void)
{
	QApplication::beep();
}


/* Set the program's subtitle. A NULL subtitle is equivalent to the
 * empty string. The subtitle is displayed in the window dressing (if
 * any).
 */
void setsubtitle(char const *subtitle)
{
	g_pMainWnd->SetSubtitle(subtitle);
}

void TileWorldMainWnd::SetSubtitle(const char* szSubtitle)
{
	QString sTitle = TileWorldApp::s_sTitle;
	if (szSubtitle && *szSubtitle)
		sTitle += QStringLiteral(" - ") + TWTextCoder::decode(szSubtitle);
	setWindowTitle(sTitle);
}


/* Display a message to the user. cfile and lineno can be NULL and 0
 * respectively; otherwise, they identify the source code location
 * where this function was called from. prefix is an optional string
 * that is displayed before and/or apart from the body of the message.
 * fmt and args define the formatted text of the message body. action
 * indicates how the message should be presented. NOTIFY_LOG causes
 * the message to be displayed in a way that does not interfere with
 * the program's other activities. NOTIFY_ERR presents the message as
 * an error condition. NOTIFY_DIE should indicate to the user that the
 * program is about to shut down.
 */
void usermessage(int action, char const *prefix,
                 char const *cfile, unsigned long lineno,
                 char const *fmt, va_list args)
{
	fprintf(stderr, "%s: ", action == NOTIFY_DIE ? "FATAL" :
	                        action == NOTIFY_ERR ? "error" : "warning");
	if (prefix)
		fprintf(stderr, "%s: ", prefix);
	if (fmt)
		vfprintf(stderr, fmt, args);
	if (cfile)
		fprintf(stderr, " [%s:%lu] ", cfile, lineno);
    fputc('\n', stderr);
    fflush(stderr);
}


/* Displays a screenful of (hopefully) helpful information which
 * includes tile images. title provides the title of the display. rows
 * points to an array of tiletablerow structures. count specifies the
 * size of this array. The text of each row is displayed alongside one
 * or two tile images. completed controls the prompt that the user
 * sees at the bottom of the display. A positive value will indicate
 * that more text follows. A negative value will indicate that leaving
 * this screen will return to the prior display. A value of zero will
 * indicate that the current display is the end of a sequence.
 */
int displaytiletable(char const *title, tiletablerow const *rows,
			    int count, int completed)
{
	// TODO
	return true;
}


/* Displays a screenful of (hopefully) helpful information. title
 * provides the title of the display. table points to a table that
 * contains the body of the text. completed controls the prompt that
 * the user sees at the bottom of the display; see the description of
 * displaytiletable() for details.
 */
int displaytable(char const *title, tablespec const *table,
			int completed)
{
	// TODO
	return true;
}

int getselectedruleset()
{
	return g_pMainWnd->GetSelectedRuleset();
}

int TileWorldMainWnd::GetSelectedRuleset()
{
	return m_pRadioMs->isChecked() ? Ruleset_MS : Ruleset_Lynx;
}

/* Read any additional data for the series.
 */
void readextensions(gameseries *series)
{
	if (g_pMainWnd == nullptr)
		return;	// happens during batch verify, etc.
	g_pMainWnd->ReadExtensions(series);
}

void TileWorldMainWnd::ReadExtensions(gameseries* pSeries)
{
	QDir dataDir(QString::fromLocal8Bit(seriesdatdir));
	QString sSetName = QFileInfo(QString::fromLocal8Bit(pSeries->mapfilename)).completeBaseName();
	QString sFilePath = dataDir.filePath(sSetName + QStringLiteral(".ccx"));
	
	m_ccxLevelset.Clear();
	if (!m_ccxLevelset.ReadFile(sFilePath, pSeries->count))
		warn("%s: failed to read file", TWTextCoder::encode(sFilePath).constData()); //is this printing? this should not be spitting out latin-1 OR windows-1252 if it is
		
	for (int i = 1; i <= pSeries->count; ++i)
	{
		CCX::Level& rCCXLevel = m_ccxLevelset.vecLevels[i];
		rCCXLevel.txtPrologue.bSeen = false;	// @#$ (pSeries->games[i-1].sgflags & SGF_HASPASSWD) != 0;
		rCCXLevel.txtEpilogue.bSeen = false;
	}
}


void TileWorldMainWnd::Narrate(CCX::Text CCX::Level::*pmTxt, bool bForce)
{
	CCX::Text& rText = m_ccxLevelset.vecLevels[m_nLevelNum].*pmTxt;
	if ((rText.bSeen || !action_displayCCX->isChecked()) && !bForce)
		return;
	rText.bSeen = true;

	if (rText.vecPages.empty())
		return;
	int n = rText.vecPages.size();

	QString sWindowTitle = this->windowTitle();
	SetSubtitle("");	// TODO: set name
	SetCurrentPage(PAGE_TEXT);
	m_pBtnTextNext->setFocus();
	
	int d = +1;
	for (int nPage = 0; nPage < n; nPage += d)
	{
		m_pBtnTextPrev->setVisible(nPage > 0);
		
		CCX::Page& rPage = rText.vecPages[nPage];
		
		m_pTextBrowser->setAlignment(rPage.pageProps.align | rPage.pageProps.valign);
		// TODO: not working!
		
		// m_pTextBrowser->setTextBackgroundColor(rPage.pageProps.bgcolor);
		// m_pTextBrowser->setTextColor(rPage.pageProps.color);
		QPalette pal = m_pTextBrowser->palette();
		pal.setColor(QPalette::Base, rPage.pageProps.bgcolor);
		pal.setColor(QPalette::Text, rPage.pageProps.color);
		m_pTextBrowser->setPalette(pal);

		QTextDocument* pDoc = m_pTextBrowser->document();
		if (pDoc != nullptr)
		{
			if (!m_ccxLevelset.sStyleSheet.isEmpty())
				pDoc->setDefaultStyleSheet(m_ccxLevelset.sStyleSheet);
			pDoc->setDocumentMargin(16);
		}

		QString sText = rPage.sText;
		if (rPage.pageProps.eFormat == CCX::TEXT_PLAIN)
		{
			/*
			sText.replace("&", "&amp;");
			sText.replace("<", "&lt;");
			sText.replace(">", "&gt;");
			sText.replace("\n", "<br>");
			m_pTextBrowser->setHtml(sText);
			*/
			m_pTextBrowser->setPlainText(sText);
		}
		else
		{
			m_pTextBrowser->setHtml(sText);
		}
		
		d = g_pApp->exec();
		if (m_bWindowClosed) g_pApp->ExitTWorld();
		if (d == 0)	// Return
			break;
		if (nPage+d < 0)
			d = 0;
	}

	SetCurrentPage(PAGE_GAME);
	setWindowTitle(sWindowTitle);
}

void TileWorldMainWnd::ShowAbout()
{
	QString text;
	int const numlines = vourzhon->rows;
	for (int i = 0; i < numlines; ++i)
	{
		if (i > 0)
			text += QStringLiteral("\n\n");
		char const *item = vourzhon->items[2*i + 1];
		text += TWTextCoder::decode(item + 2);  // skip over formatting chars
	}
	QMessageBox::about(this, tr("About"), text);
}

void TileWorldMainWnd::OnTextNext()
{
	g_pApp->exit(+1);
}

void TileWorldMainWnd::OnTextPrev()
{
	g_pApp->exit(-1);
}

void TileWorldMainWnd::OnTextReturn()
{
	g_pApp->exit(0);
}


void TileWorldMainWnd::OnCopyText()
{
	QClipboard* pClipboard = QApplication::clipboard();
	if (pClipboard == nullptr)
		return;
	pClipboard->setText(m_sTextToCopy);
}

void TileWorldMainWnd::OnMenuActionTriggered(QAction* pAction)
{
	if (pAction == action_Prologue)
	    { Narrate(&CCX::Level::txtPrologue, true); return; }
	if (pAction == action_Epilogue)
	    { Narrate(&CCX::Level::txtEpilogue, true); return; }

	if (pAction == action_displayCCX)
	{
	    setintsetting("displayccx", pAction->isChecked() ? 1 : 0);
	    return;
	}

	if (pAction == action_forceShowTimer)
	{
	    setintsetting("forceshowtimer", pAction->isChecked() ? 1 : 0);
		drawscreen(TRUE);
	    return;
	}

	if (pAction == action_About)
	{
		ShowAbout();
		return;
	}

	int nTWKey = GetTWKeyForAction(pAction);
	if (nTWKey == TWK_dummy) return;
	PulseKey(nTWKey);
}

int TileWorldMainWnd::GetTWKeyForAction(QAction* pAction) const
{
    if (pAction == action_Scores) return TWC_SEESCORES;
    if (pAction == action_SolutionFiles) return TWC_SEESOLUTIONFILES;
    if (pAction == action_TimesClipboard) return TWC_TIMESCLIPBOARD;
    if (pAction == action_Levelsets) return TWC_QUITLEVEL;
    if (pAction == action_Exit) return TWC_QUIT;

    if (pAction == action_Begin) return TWC_PROCEED;
    if (pAction == action_Pause) return TWC_PAUSEGAME;
    if (pAction == action_Restart) return TWC_SAMELEVEL;
    if (pAction == action_Next) return TWC_NEXTLEVEL;
    if (pAction == action_Previous) return TWC_PREVLEVEL;
    if (pAction == action_GoTo) return TWC_GOTOLEVEL;

    if (pAction == action_Playback) return TWC_PLAYBACK;
    if (pAction == action_Verify) return TWC_CHECKSOLUTION;
    if (pAction == action_Replace) return TWC_REPLSOLUTION;
    if (pAction == action_Delete) return TWC_KILLSOLUTION;

    if (pAction == action_Keys) return TWC_KEYS;
    return TWK_dummy;
}

bool TileWorldMainWnd::SetHintMode(HintMode newmode)
{
	bool changed = (newmode != m_hintMode);
	m_hintMode = newmode;
	return changed;
}
