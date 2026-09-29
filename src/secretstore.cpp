#include "secretstore.h"

#include <QByteArray>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#include <QHeaderView>
#include <QJsonArray>
#include <QTableWidget>
#include <QScrollBar>
#endif

namespace {

const char kPrefix[] = "enc:v1:";

} // namespace

#ifdef Q_OS_WIN

namespace {

bool dpapiProtect(const QByteArray &plain, QByteArray *out)
{
    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(plain.constData()));
    in.cbData = static_cast<DWORD>(plain.size());

    DATA_BLOB encrypted;
    if (!CryptProtectData(&in, L"SimpleAIClient", nullptr, nullptr, nullptr, 0, &encrypted)) {
        return false;
    }

    out->clear();
    out->append(reinterpret_cast<const char *>(encrypted.pbData), static_cast<int>(encrypted.cbData));
    LocalFree(encrypted.pbData);
    return true;
}

bool dpapiUnprotect(const QByteArray &cipher, QByteArray *out)
{
    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(cipher.constData()));
    in.cbData = static_cast<DWORD>(cipher.size());

    DATA_BLOB plain;
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &plain)) {
        return false;
    }

    out->clear();
    out->append(reinterpret_cast<const char *>(plain.pbData), static_cast<int>(plain.cbData));
    LocalFree(plain.pbData);
    return true;
}

} // namespace

bool SecretStore::isEncrypted(const QByteArray &stored)
{
    return stored.startsWith(kPrefix);
}

QByteArray SecretStore::protectBytes(const QByteArray &plain)
{
    if (plain.isEmpty()) {
        return QByteArray();
    }

    QByteArray cipher;
    if (dpapiProtect(plain, &cipher)) {
        return QByteArray(kPrefix) + cipher.toBase64();
    }
    return plain;
}

QByteArray SecretStore::unprotectBytes(const QByteArray &stored)
{
    if (stored.isEmpty()) {
        return QByteArray();
    }
    if (!isEncrypted(stored)) {
        // Legacy plain-text value written by an older build.
        return stored;
    }

    QByteArray plain;
    if (dpapiUnprotect(QByteArray::fromBase64(stored.mid(static_cast<int>(qstrlen(kPrefix)))), &plain)) {
        return plain;
    }
    return QByteArray();
}

bool SecretStore::isEncrypted(const QString &stored)
{
    return isEncrypted(stored.toUtf8());
}

QString SecretStore::protect(const QString &plain)
{
    return QString::fromUtf8(protectBytes(plain.toUtf8()));
}

QString SecretStore::unprotect(const QString &stored)
{
    if (stored.isEmpty()) {
        return QString();
    }
    return QString::fromUtf8(unprotectBytes(stored.toUtf8()));
}

#else // !Q_OS_WIN

bool SecretStore::isEncrypted(const QByteArray &)
{
    return false;
}

bool SecretStore::isEncrypted(const QString &)
{
    return false;
}

QByteArray SecretStore::protectBytes(const QByteArray &plain)
{
    return plain;
}

QByteArray SecretStore::unprotectBytes(const QByteArray &stored)
{
    return stored;
}

QString SecretStore::protect(const QString &plain)
{
    return plain;
}

QString SecretStore::unprotect(const QString &stored)
{
    return stored;
}

#endif // Q_OS_WIN

const char *SecretStore::prefix()
{
    return kPrefix;
}
