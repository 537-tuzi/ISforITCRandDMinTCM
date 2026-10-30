#ifndef SPEAK_H
#define SPEAK_H
// 语音播报（TTS）：需启用 .pro 中的 PATIENT_TTS 宏与 Qt TextToSpeech 模块
#include <QString>

#ifdef PATIENT_TTS
#include <QTextToSpeech>
#include <QLocale>
inline void speakText(const QString& text)
{
    static QTextToSpeech spk;
    spk.setLocale(QLocale::Chinese);
    spk.say(text);
}
#else
inline void speakText(const QString& text) { Q_UNUSED(text); }
#endif

#endif
