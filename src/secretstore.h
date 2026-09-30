#ifndef SECRETSTORE_H
#define SECRETSTORE_H

#include <QByteArray>
#include <QString>

// Encrypted-at-rest storage for anything that must not sit in plain text in
// QSettings: the Venice API key, the per-profile keys, and the chat history
// (which contains the prompts and any base64 image attachments).
//
// On Windows the value is wrapped with DPAPI (CryptProtectData /
// CryptUnprotectData) scoped to the current user, so a registry dump or a
// stolen backup file does not expose the contents. Encrypted values carry an
// "enc:v1:" prefix; unprefixed values are legacy plain text and are still
// readable, so existing installations migrate without losing data.
//
// Non-Windows builds, and any call where the Windows API fails, fall back to
// storing the plain text so the app keeps working.
class SecretStore
{
public:
    static QString protect(const QString &plain);
    static QString unprotect(const QString &stored);

    // Same encoding, for payloads that are not valid UTF-8 text or that are
    // large enough that QString's UTF-16 round trip is wasteful.
    static QByteArray protectBytes(const QByteArray &plain);
    static QByteArray unprotectBytes(const QByteArray &stored);

    static bool isEncrypted(const QString &stored);
    static bool isEncrypted(const QByteArray &stored);
};

#endif // SECRETSTORE_H
