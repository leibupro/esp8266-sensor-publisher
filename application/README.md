# Readme

## `passwd.h` header

File is intentionally ignored. One must create this file before
the first build in the directory [`include`](include). The content
of `passwd.h` is just a single text string of either the Wifi
password in plain text, e.g. `"foobar"`, or a hex string, e.g.
`"771f79b2ea17b2064c50076b9a6c19ac0f7972abf16ba5e5809e604c027883d5"`,
representing a 32 byte hash derived from the SSID (as salt) and the
Wifi password, also known as the Pairwise Master Key (PMK). To create
such a PMK, one can use the `wpa_passphrase` tool, i.e.:

```console
usage: wpa_passphrase <ssid> [passphrase]
$ wpa_passphrase "foo-bar-baz-net" "smileandwave"
```

