#pragma once

#include <QLocale>
#include <QString>

class QTranslator;

namespace UiLanguage {
inline constexpr auto settingKey = "ui/language";

QString preference();
QString resolve(const QString& preference, const QLocale& systemLocale = QLocale::system());
bool install(QTranslator& translator, const QString& language);
}
