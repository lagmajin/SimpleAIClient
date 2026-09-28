#ifndef CREDENTIALSTORE_H
#define CREDENTIALSTORE_H

#include <QString>

// Stores secrets (currently just the Venice API key and its per-profile
// variants) encrypted at rest.
//
// On Windows the value is wrapped with DPAPI (CryptProtectData /
// CryptUnprotectData) scoped to the current user, so a registry dump or a
// stolen backup file does not expose the key in plain text. Elsewhere, and if
// the Windows call fails, the value falls back to plain text with a
// "enc:" prefix that is never emitted for unencrypted legacy values, keeping
// existing installations readable.
class CredentialStore
{
public:
    static QString protect(const QString &plain);
    static QString unprotect(const QString &stored);

    static bool isEncrypted(const QString &stored);
};

#endif // CREDENTIALSTORE_H
