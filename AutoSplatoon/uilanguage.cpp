#include "uilanguage.h"

#include <QCoreApplication>
#include <QSettings>
#include <QTranslator>

QString UiLanguage::preference()
{
    const QString value = QSettings().value(settingKey).toString();
    return value == "en" || value == "zh_CN" ? value : QStringLiteral("system");
}

QString UiLanguage::resolve(const QString& preference, const QLocale& systemLocale)
{
    if (preference == "en" || preference == "zh_CN") return preference;
    const auto languages = systemLocale.uiLanguages();
    const QLocale uiLocale = languages.isEmpty() ? systemLocale : QLocale(languages.first());
    return uiLocale.language() == QLocale::Chinese ? QStringLiteral("zh_CN") : QStringLiteral("en");
}

bool UiLanguage::install(QTranslator& translator, const QString& language)
{
    const QString resource = language == "en" ? QStringLiteral(":/i18n/autosplatoon_en.qm")
                                               : QStringLiteral(":/i18n/qtbase_zh_CN.qm");
    return translator.load(resource) && QCoreApplication::installTranslator(&translator);
}
