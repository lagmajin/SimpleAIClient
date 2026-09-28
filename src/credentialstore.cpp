#include "credentialstore.h"

#include <QByteArray>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#endif

namespace {

QString toBase64(const QByteArray &data)
{
    return QString::fromLatin1(data.toBase64());
}

QByteArray fromBase64(const QString &text)
{
    return QByteArray::fromBase64(text.toLatin1());
}

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

bool CredentialStore::isEncrypted(const QString &stored)
{
    return stored.startsWith(QLatin1String("enc:v1:"));
}

QString CredentialStore::protect(const QString &plain)
{
    if (plain.isEmpty()) {
        return QString();
    }

    QByteArray cipher;
    if (dpapiProtect(plain.toUtf8(), &cipher)) {
        return QLatin1String("enc:v1:") + toBase64(cipher);
    }
    return plain;
}

QString CredentialStore::unprotect(const QString &stored)
{
    if (stored.isEmpty()) {
        return QString();
    }
    if (!isEncrypted(stored)) {
        // Legacy plain-text value written by an older build.
        return stored;
    }

    const QString payload = stored.mid(7);
    QByteArray plain;
    if (dpapiUnprotect(fromBase64(payload), &plain)) {
        return QString::fromUtf8(plain);
    }
    return QString();
}

#else // !Q_OS_WIN

bool CredentialStore::isEncrypted(const QString &)
{
    return false;
}

QString CredentialStore::protect(const QString &plain)
{
    return plain;
}

QString CredentialStore::unprotect(const QString &stored)
{
    return stored;
}

#endif // Q_OS_WIN
