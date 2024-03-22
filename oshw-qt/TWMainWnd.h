/* Copyright (C) 2001-2017 by Madhav Shanbhag and Eric Schmidt,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef TWMAINWND_H
#define TWMAINWND_H


#include "ui_TWMainWnd.h"

#include "CCMetaData.h"

#include "../generic/generic.h"

#include "../gen.h"
#include "../defs.h"
#include "../state.h"
#include "../series.h"
#include "../oshw.h"

#include <QMainWindow>

#include <QLocale>
#include <QThread>

#include "TWSfx.h"

class QSortFilterProxyModel;
class QAudioOutput;
class QSoundEffect;
class QAudioSink;
class QFile;

class TileWorldMainWnd : public QMainWindow, protected Ui::TWMainWnd
{
	Q_OBJECT
	
public:
	enum Page
	{
		PAGE_GAME,
		PAGE_TABLE,
		PAGE_TEXT
	};

	TileWorldMainWnd(QWidget* pParent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
	~TileWorldMainWnd();

	bool eventFilter(QObject* pObject, QEvent* pEvent) override;
	void closeEvent(QCloseEvent* pCloseEvent) override;
	void timerEvent(QTimerEvent*) override;

	bool SetKeyboardRepeat(bool bEnable);
	uint8_t* GetKeyState(int* pnNumKeys);
	int GetReplaySecondsToSkip() const;
	
	bool CreateGameDisplay();
	void ClearDisplay();
	bool DisplayGame(const gamestate* pState, int nTimeLeft, int nBestTime, bool showinitgamestate);
	bool SetDisplayMsg(const char* szMsg, int nMSecs, int nBoldMSecs);
	int DisplayEndMessage(int nBaseScore, int nTimeScore, long lTotalScore, int nCompleted);
	int DisplayList(const char* szTitle, const tablespec* pTableSpec, int* pnIndex,
			DisplayListType eListType, int (*pfnInputCallback)(int*));
	int DisplayInputPrompt(const char* szPrompt, char* pInput, int nMaxLen,
			InputPromptType eInputType, int (*pfnInputCallback)());
	int GetSelectedRuleset();
	void SetSubtitle(const char* szSubtitle);

    void EnableAudio(bool bEnabled);
    bool LoadSoundEffect(int index, const char* szFilename);
    void FreeSoundEffect(int index);
    void SetSoundEffects(unsigned long sfx);
    void StopSoundEffects();
    void SetAudioVolume(qreal fVolume);
    qreal GetAudioVolume() const { return m_volume; };
    void PauseSoundEffects(bool pause);

	void ReadExtensions(gameseries* pSeries);
	void Narrate(CCX::Text CCX::Level::*pmTxt, bool bForce = false);
	
	void ShowAbout();

private slots:
	void OnListItemActivated(const QModelIndex& index);
	void OnFindTextChanged(const QString& sText);
	void OnFindReturnPressed();
	void OnRulesetSwitched(bool mschecked);
	void OnPlayback();
	void OnSpeedValueChanged(int nValue);
	void OnSpeedSliderReleased();	
	void OnSeekPosChanged(int nValue);
	void OnTextNext();
	void OnTextPrev();
	void OnTextReturn();
	void OnCopyText();
	void OnMenuActionTriggered(QAction* pAction);
	
private:
	bool HandleEvent(QObject* pObject, QEvent* pEvent);
	void SetCurrentPage(Page ePage);
	void CheckForProblems(const gamestate* pState);
	void DisplayMapView(const gamestate* pState);
	void DisplayShutter();
	void SetSpeed(int nValue);
	void ReleaseAllKeys();
	void PulseKey(int nTWKey);
	int GetTWKeyForAction(QAction* pAction) const;
    void InitAudioThread(bool status);

	enum HintMode { HINT_EMPTY, HINT_TEXT, HINT_INITSTATE };
	bool SetHintMode(HintMode newmode);

	bool m_bSetupUi;
	bool m_bWindowClosed;
	
	Qt_Surface* m_pSurface;
	Qt_Surface* m_pInvSurface;
	TW_Rect m_disploc;

    QThread m_sfxThread;
    TWSfxManager* m_sfxManager = nullptr;
    qreal m_volume;

	uint8_t m_nKeyState[TWK_LAST];

	struct MessageData{ QString sMsg; uint32_t nMsgUntil, nMsgBoldUntil; };
	QVector<MessageData> m_shortMessages;
	
	bool m_bKbdRepeatEnabled;

	int m_nRuleset;
	int m_nLevelNum;
	bool m_bProblematic;
	bool m_bOFNT;
	int m_nBestTime;
	HintMode m_hintMode;
	int m_nTimeLeft;
	bool m_bTimedLevel;
	bool m_bReplay;
    QString m_title;
    QString m_author;

	QSortFilterProxyModel* m_pSortFilterProxyModel;
	QLocale m_locale;
	
	CCX::Levelset m_ccxLevelset;
	
	QString m_sTextToCopy;
signals:
    void enableAudio(bool bEnabled);
    void loadSoundEffect(int index, QString szFilename);
    void freeSoundEffect(int index);
    void setSoundEffects(unsigned long sfx);
    void stopSoundEffects();
    void setAudioVolume(qreal fVolume);
    void pauseSoundEffects(bool paused);
};


#endif
