#ifndef TWSFX_H
#define TWSFX_H

#include <QObject>
#include <QAudio>
#include <QAudioFormat>
#include <QAudioDecoder>
#include <QVector>
#include <QIODevice>

class QAudioSink;
class QBuffer;

/* Some generic default settings for the audio output.
 */
#define DEFAULT_SND_FREQ	22050
#define DEFAULT_SND_CHAN	1
#define DEFAULT_SND_FORM	QAudioFormat::Int16

class TWSfx : public QObject
{
    Q_OBJECT
public:
    TWSfx(QString const& filename, bool repeating, QObject* parent=nullptr);

    qint64 pos;
    bool playing;

    bool repeating;
    bool finishedDecoding;
    char* bytes;
    qint64 len;
public slots:
    void consumeConversionBuffer();
    void finishConvertingSound();
    void handleConvertionError(QAudioDecoder::Error err);
private:
    QAudioDecoder* decoder;
    QBuffer* buf;
};

class TWSoundMixer: public QIODevice {
public:
    TWSoundMixer(QObject* parent = nullptr);
    bool isSequential() const;

    void loadSoundEffect(int index, QString szFilename);
    QVector<TWSfx*> sounds;
    bool paused;

    static constexpr QAudioFormat defaultFormat() {
        QAudioFormat format = QAudioFormat();
        format.setSampleRate(DEFAULT_SND_FREQ);
        format.setChannelCount(DEFAULT_SND_CHAN);
        format.setSampleFormat(DEFAULT_SND_FORM);
        return format;
    }
protected:
    qint64 readData(char* data, qint64 len);
    qint64 writeData(const char* data, qint64 len);
};

class TWSfxManager: public QObject {
    Q_OBJECT
public:
    TWSfxManager(QObject* parent = nullptr);
    ~TWSfxManager();
private:
    bool enableAudio;
    QAudioSink* sink;
    TWSoundMixer* mixer;

public slots:
    void EnableAudio(bool bEnabled);
    void LoadSoundEffect(int index, QString szFilename);
    void SetSoundEffects(unsigned long sfx);
    void StopSoundEffects();
    void SetAudioVolume(qreal fVolume);
    void PauseSoundEffects(bool pause);

};

#endif // TWSFX_H
