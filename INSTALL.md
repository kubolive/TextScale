# Installation Guide – TextScale Plugin for OBS Studio

This guide provides step-by-step instructions for installing and activating the **TextScale** video filter plugin in OBS Studio on Windows 10 / Windows 11 (64-bit).

---

## Supported Versions

- **OBS Studio:** Version 30.0.0 or higher (Tested on OBS Studio 31.1.2 x64).
- **Operating System:** Windows 10 x64 / Windows 11 x64.
- **Prerequisites:** Visual C++ 2015-2022 Redistributable (x64) installed (standard requirement for OBS Studio).

---

## Method 1: User Profile Installation (Recommended)

This method does **not** require administrator privileges and keeps the plugin isolated within your user account.

1. Open File Explorer or press `Win + R`.
2. Type `%APPDATA%\obs-studio\plugins` and press **Enter**.  
   *(If the `plugins` folder does not exist, create it).*
3. Create a new subfolder named: `text-scale`.
4. Copy the plugin files so the directory structure looks exactly like this:

```
%APPDATA%\obs-studio\plugins\text-scale\
├── bin\
│   └── 64bit\
│       ├── text-scale.dll
│       └── libwinpthread-1.dll
└── data\
    ├── text_scale.effect
    └── locale\
        └── en-US.ini
```

---

## Method 2: System-Wide Installation (All Users)

For multi-user broadcast workstations:

1. Open File Explorer as Administrator.
2. Navigate to: `C:\ProgramData\obs-studio\plugins\`  
   *(Note: `ProgramData` is a hidden folder by default).*
3. Create a subfolder named `text-scale` and place the `bin` and `data` directories inside:

```
C:\ProgramData\obs-studio\plugins\text-scale\
├── bin\
│   └── 64bit\
│       ├── text-scale.dll
│       └── libwinpthread-1.dll
└── data\
    ├── text_scale.effect
    └── locale\
        └── en-US.ini
```

---

## Verifying the Installation

1. Launch **OBS Studio (64-bit)**.
2. In the top menu, go to **Help → Log Files → View Current Log**.
3. Search the log file (`Ctrl + F`) for `TextScale`. You should see:
   ```
   [TextScale] Plugin version 1.0.0 loaded successfully
   ...
   Loaded Modules:
     text-scale.dll
   ```
4. In your scene, select any **Text (GDI+)** source.
5. Right-click the source and select **Filters** (or click the *Filters* button in the action bar).
6. Under **Effect Filters**, click the **+** (Add) button.
7. Select **TextScale** from the dropdown menu and click **OK**.

---

## Quick Configuration Checklist

- **For Automated News Ticker / Lower-Third:**
  1. Open the TextScale filter properties on your text source.
  2. Check **Enable Auto Condense**.
  3. Set **Target Width** to match your graphic bar width (e.g., `267 px`, `450 px`, or `600 px`).
  4. Ensure **Minimum Scale** is set to `65%` (prevents text from becoming illegibly narrow).
  5. Leave **Preserve Height** enabled (`On`).
  6. Choose your **Alignment** (`Left`, `Center`, or `Right`).
  7. When long breaking news or headlines are entered, the text automatically condenses smoothly without altering font size!

---

## How to Uninstall

1. Close OBS Studio completely.
2. Delete the `text-scale` folder from:
   - `%APPDATA%\obs-studio\plugins\text-scale\` (if installed for the current user), or
   - `C:\ProgramData\obs-studio\plugins\text-scale\` (if installed for all users).
3. Restart OBS Studio.
