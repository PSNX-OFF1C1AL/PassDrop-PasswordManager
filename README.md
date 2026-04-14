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

1. **Title**-Account name (e.g., `Google`)  
2. **URL**-Website address (`https://google.com`)  
3. **Login**-Username or email (`myemail@gmail.com`)  
4. **Password**-Account password (`MySecretPass123`)  
5. **Extra 1 Label**-Name of first custom field (`2FA Code`)  
6. **Extra 1 Data**-Value of first custom field (`JBSWY3DPEHPK3PXP`)  
7. **Extra 2 Label**-Name of second custom field (`PIN`)  
8. **Extra 2 Data**-Value of second custom field (`1234`)  
9. **Icon**-Emoji or symbol (`*`)  
10. **Color**-Color index 0–6 (`2` for Orange)

**Example CSV row:**  
Title,URL,Login,Password,Extra1Label,Extra1Data,Extra2Label,Extra2Data,Icon,Color
`"Google","https://google.com","myemail@gmail.com","MySecretPass123","2FA Code","JBSWY3DPEHPK3PXP","PIN","1234","*","2"`

**Color Index:**  
0=Default (White)  
1=Red  
2=Orange  
3=Yellow  
4=Green  
5=Blue  
6=Purple  

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

**Title**-The name of the account (e.g., `Google`, `Steam`). Double‑click opens the website.  
**Login**-Username or email. Click `Copy` to grab it quickly.  
**Password**-Hidden by default. Use `Show` to reveal, `Copy` to copy.  
**Extra 1**-A custom field-you can rename it (for example, `2FA code` or `Recovery email`).  
**Extra 2**-Another custom field for anything else you need to remember.  
**Actions**-Buttons to `Edit`, `Delete`, or open the website directly.

Every row can have its own color tag and icon-helpful when you have dozens of accounts.

## 📝 How to use PassDrop 

PassDrop is designed to be simple, but here's a full walkthrough of every button and field so nothing is left to guesswork.

### The Top Bar (Window Controls)

These buttons control the application window itself.

-**`▲` / `△` (Always on Top)**-Toggles the "Always on Top" mode. When the triangle is filled (`▲`), PassDrop stays visible above all other windows. Super handy when you're copying passwords into a website or app. Click it again (empty triangle `△`) to let it behave like a normal window.
-**`_` (Minimize)**-Sends PassDrop to the system tray (the little icons near the clock). It's still running, just out of your way. Click the PassDrop icon in the tray to bring it back.
-**`[ ]` / `[_]` (Maximize / Restore)**-Makes PassDrop fill the screen or returns it to its previous size. The icon changes depending on the current state.
-**`X` (Close)**-Quits PassDrop completely. Any changes you made are already saved, so you don't need to worry about losing data.

### The Toolbar (Main Actions)

These are the primary actions for managing your password vault.

-**`+ Add Entry`**-Opens the dialog where you can create a new account entry. See the section below for details on each field.
-**`Search...` bar**-Start typing here to filter the list of accounts. It matches against the **Title** field. The matching text is highlighted in yellow for easy spotting.
-**`New`**-Creates a brand new, empty vault. **Use with caution!** This will clear the current list. You'll be asked to confirm first, and you should save your old vault somewhere safe if you want to keep it.
-**`Open`**-Opens a file dialog to load an existing `.pvault` file. Useful if you have multiple vaults or are moving one from another computer.
-**`Save As`**-Saves the current vault to a new `.pvault` file. Use this to create backups or save a copy in a different location.
-**`Import`**-Reads account data from a `.csv` file and adds it to your current vault. Make sure the CSV file follows the exact format described in the "CSV Format" section above.
-**`Export`**-Saves all your accounts from the current vault into a `.csv` file. **Heads up:** The exported file is **not encrypted**, so handle it with care and delete it when you're done.
-**`About`**-Shows the program version, license, and author information.

### The Main Table (Your Accounts)

This is where all your stored accounts are listed.

-**Title**-The name you've given to the account (e.g., `Google`, `Steam`).
    -**Double-click** any title to open its associated URL in your default web browser.
-**Login**-The username or email for the account.
    -Click the **`Copy`** button next to it to copy the login to your clipboard.
-**Password**-The password for the account. It's hidden by default for privacy.
    -Click **`Show`** to reveal the password temporarily.
    -Click **`Copy`** to copy the password to your clipboard (it will be automatically cleared after 30 seconds).
-**Extra 1 / Extra 2**-These are fully customizable fields. You can change their labels in the Add/Edit dialog. Use them for anything extra: 2FA recovery codes, security question answers, PINs, old passwords, or just notes.
    -If a field has data, a **`Copy`** button will appear next to it.
-**Actions**-The buttons to manage an individual account.
    -**`Edit`**-Opens the dialog to change any of the account's details.
    -**`Del`**-Deletes the account. You'll be asked to confirm before it's permanently removed.
    -**`URL`**-This button only appears if you've saved a URL for the account. Click it to quickly open the website in your browser.

### The Add / Edit Dialog

This dialog appears when you click `+ Add Entry` or the `Edit` button on an existing account.

-**Title**-A descriptive name for the account (required).
-**URL**-The full website address (optional). If provided, double-clicking the title in the main table will open this link.
-**Login**-The username or email (optional).
-**Password**-The account password (optional).
    -The bar below the field shows the estimated password strength.
    -Click the **`Generator...`** button to open a tool for creating strong, random passwords with customizable length and character sets.
-**Icon**-A simple symbol or emoji to help visually identify the account in the list.
-**Color**-A color tag for the account row in the main table. Useful for categorizing (e.g., red for banking, green for social media).
-**Extra 1 Label / Data**-The custom name and value for the first extra field.
-**Extra 2 Label / Data**-The custom name and value for the second extra field.
-**`OK`**-Saves the account and closes the dialog.
-**`Cancel`**-Closes the dialog without saving any changes.

---

## 🤝 Support & Contributions

If you find PassDrop useful, here are some ways you can help:

🐛 Report a bug: Telegram @PichanDev or [X (twitter)](https://x.com/PichanTsu)
💡 Suggest an idea: I'm always open to ideas!  
⭐ Star the repo-it helps others discover the project  
💬 Spread the word-tell a friend, share on social media  
☕ Financial support: [DonationAlerts](https://www.donationalerts.com/r/pichan)-completely optional, but greatly appreciated!

I'm also open to collaboration, feedback, or just a friendly chat-about PassDrop or anything else. Feel free to reach out!

---

*Made with ☕, late nights, and a genuine desire to build something useful. (Full disclosure: there were plenty of mistakes, I got angry more times than I can count, but I'll be genuinely happy if this little app helps someone.)*

---

## 🛠️ Technical Details

**Language:** C++20  
**UI Framework:** Dear ImGui (DirectX 11 backend)  
**Encryption:** XOR-based with a static key embedded in the binary  
**Storage:** Single encrypted `.pvault` file (SQLite-like custom binary format)  
**Dependencies:** None beyond standard Windows system libraries (statically linked)  

**Build Requirements (if compiling from source):**
-Visual Studio 2022 or newer
-vcpkg with `imgui[core,dx11-binding,win32-binding]`
-Windows SDK 10.0+

**Repository Structure:**
-`main.cpp`-application entry point and main UI loop
-`DataStore.cpp`-vault file read/write and encryption layer
-`CryptoService.cpp`-encryption/decryption and password generation
-`Account.h`-data model for password entries
-`AddEditDialog.cpp`-modal dialog for adding/editing entries

**Why no cloud sync?**
PassDrop is designed to be offline-first. If you need sync, copy the `.pvault` file manually or use a third-party sync tool of your choice.

**Why XOR encryption?**
It's simple, fast, and keeps your data safe from casual snooping. It won't stop a determined attacker with access to your machine, but that's not the threat model PassDrop is built for. If you need military-grade encryption, there are other tools for that.
