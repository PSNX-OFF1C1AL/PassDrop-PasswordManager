# PassDrop-simple, offline, and honest password manager

## 📢 Note: This is v1.0

PassDrop is brand new-this is its first public release!

I've tested it in many different scenarios, and it works well for everyday use. But like any version 1.0 software, there may still be a few undiscovered bugs hiding somewhere.

Please be a little careful:
-Backup your vault file from time to time (just copy `default.pvault` somewhere safe).
-Report any strange behavior-I'll try to fix it.

Thank you for trying PassDrop this early. You're awesome! 💙

Hi! I'm Maksutov Vladislav Vitalievich (Pichan), the developer of PassDrop.

I built PassDrop because I wanted a password manager that:
-works completely offline,
-is simple-no complicated settings, just open and use,
-looks modern-dark theme by default, clean interface,
-and respects your privacy-your data never leaves your computer.

PassDrop is free and open source (GPL v3).  
You can use it forever without paying a cent, and you can even look inside to see how it works.

---

## 🔐 What PassDrop does (and doesn't do)

✅ Stores your passwords locally in an encrypted file  
   *Basic encryption keeps your data safe from casual snooping (99% of everyday situations).*

✅ Auto-saves every change-you'll never lose anything  
   *Changes are saved instantly. Data loss is extremely rare (<0.1% chance, usually only during power outages). A quick backup never hurts!*

✅ Clears the clipboard 30 seconds after you copy a password  
   *In 95% of cases, this keeps your passwords safe. Some apps (like clipboard managers) may keep history-nothing to worry about for most users.*

✅ Imports and exports CSV-easy migration from other managers  
   *Works great with most password managers. Just match the column order (see format below) and you're all set.*

### 📄 CSV Format

When importing or exporting, PassDrop expects the following column order:

1. **Title** – Account name (e.g., `Google`)  
2. **URL** – Website address (`https://google.com`)  
3. **Login** – Username or email (`myemail@gmail.com`)  
4. **Password** – Account password (`MySecretPass123`)  
5. **Extra 1 Label** – Name of first custom field (`2FA Code`)  
6. **Extra 1 Data** – Value of first custom field (`JBSWY3DPEHPK3PXP`)  
7. **Extra 2 Label** – Name of second custom field (`PIN`)  
8. **Extra 2 Data** – Value of second custom field (`1234`)  
9. **Icon** – Emoji or symbol (`*`)  
10. **Color** – Color index 0–6 (`2` for Orange)

**Example CSV row:**  
Title,URL,Login,Password,Extra1Label,Extra1Data,Extra2Label,Extra2Data,Icon,Color
`"Google","https://google.com","myemail@gmail.com","MySecretPass123","2FA Code","JBSWY3DPEHPK3PXP","PIN","1234","*","2"`

**Color Index:**  
0 = Default (White)  
1 = Red  
2 = Orange  
3 = Yellow  
4 = Green  
5 = Blue  
6 = Purple  

*Extra fields can be left empty if you don't need them.*

✅ Works offline-no internet connection required

✅ Supports English, Russian, and Japanese input  
   *99% of users will never notice any limits. Rare symbols from other languages might not appear-but your passwords will still work perfectly.*

✅ Stays on top of other windows when you need it  
   *Handy 99% of the time. If a file dialog hides behind it, just toggle "Always on Top" off for a moment.*

✅ Minimizes to tray-out of your way, but always ready

❌ No cloud sync (your data stays with you)  
❌ No browser extensions (for security reasons)  
❌ No ads, no tracking, no analytics  

---

## 📋 What's in the main window?

When you open PassDrop, you'll see a table with these columns:

**Title** – The name of the account (e.g., `Google`, `Steam`). Double‑click opens the website.  
**Login** – Username or email. Click `Copy` to grab it quickly.  
**Password** – Hidden by default. Use `Show` to reveal, `Copy` to copy.  
**Extra 1** – A custom field-you can rename it (for example, `2FA code` or `Recovery email`).  
**Extra 2** – Another custom field for anything else you need to remember.  
**Actions** – Buttons to `Edit`, `Delete`, or open the website directly.

Every row can have its own color tag and icon-helpful when you have dozens of accounts.

---

## 🤝 Support & Contributions

If you find PassDrop useful, here are some ways you can help:

🐛 Report a bug: Telegram @PichanDev or [X (twitter)](https://x.com/PichanTsu)
💡 Suggest an idea: I'm always open to ideas!  
⭐ Star the repo — it helps others discover the project  
💬 Spread the word — tell a friend, share on social media  
☕ Financial support: [DonationAlerts](https://www.donationalerts.com/r/pichan)-completely optional, but greatly appreciated!

I'm also open to collaboration, feedback, or just a friendly chat-about PassDrop or anything else. Feel free to reach out!

---

*Made with ☕, late nights, and a genuine desire to build something useful. (Full disclosure: there were plenty of mistakes, I got angry more times than I can count, but I'll be genuinely happy if this little app helps someone.)*
