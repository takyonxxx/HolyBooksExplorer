# Holy Books Explorer / Kutsal Kitaplar Gezgini

A comprehensive Qt6 application for exploring and studying holy books including the Quran, Gospel, Torah, and Psalms.

Kuran-ı Kerim, İncil, Tevrat ve Zebur'u incelemenizi sağlayan kapsamlı bir Qt6 uygulaması.

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6.10-green.svg)

## Features / Özellikler

### 📚 Book Support / Kitap Desteği
- **Kuran-ı Kerim** - Full support with Arabic text, Latin transliteration, clean Turkish meanings, English translations, and word-by-word meanings
- **İncil (Gospel)** - Complete Gospel text organized by books and chapters
- **Tevrat (Torah)** - Torah text with chapter navigation
- **Zebur (Psalms)** - Psalms/Mezmur with verse navigation

### 🔍 Search Functionality / Arama Özellikleri
- Search across all verses in the selected book
- **Yellow highlighting** of found keywords (customizable color)
- Live search preview as you type
- Export search results to text file

### 📖 Quran Special Features / Kuran Özel Özellikleri
- **Clean Turkish meanings** (saf meal) - No added interpretations, only what's in the original text
- **English translations** for major suras
- Arabic text display (العربية)
- Latin transliteration
- **Word-by-word meanings** in Turkish and English
- Sort by **revelation order** (iniş sırasına göre)
- Sort by sura number

### ⚙️ Customization / Özelleştirme
- **Font settings** - Change font family and size (default 12px)
- **Display options** - Show/hide Arabic and Latin texts
- **Highlight color** - Customize search highlight color
- **Language support** - Turkish and English interface

### 📤 Export Features / Dışa Aktarma
- Export entire chapters to text files
- Export search results
- Copy verses to clipboard (right-click menu)

### 🗄️ Embedded Database / Gömülü Veritabanı
- Database is embedded in resources and automatically extracted on first run
- No external database file needed after build

## Requirements / Gereksinimler

- Qt 6.10 or higher
- C++17 compiler
- SQLite support (included in Qt)

## Building / Derleme

```bash
cd HolyBooksExplorer
mkdir build && cd build
qmake ../HolyBooksExplorer.pro
make -j4
./HolyBooksExplorer
```

The database will be automatically extracted to the application directory on first run.

### Windows (with Qt Creator)
1. Open `HolyBooksExplorer.pro` in Qt Creator
2. Configure the project with Qt 6.10
3. Build and run

## Database Structure / Veritabanı Yapısı

| Table | Description |
|-------|-------------|
| `tbl_kuran_sureler` | Quran suras with revelation order |
| `tbl_kuran_meal` | Quran verses with clean Turkish meaning (meal_saf), English translation (meal_en), Arabic, Latin |
| `tbl_kuran_kelime` | Word-by-word meanings for Quran |
| `tbl_incil` | Gospel verses |
| `tbl_incil_sureler` | Gospel chapters |
| `tbl_tevrat` | Torah verses |
| `tbl_tevrat_sureler` | Torah chapters |
| `tbl_zebur` | Psalms verses |
| `tbl_zebur_sureler` | Psalms chapters |

### Clean Meanings / Saf Mealler

The Quran meanings have been cleaned to remove added interpretations. For example:

**Original (with additions):**
> "Kulu Muhammed'i geceleyin..." (İsra 17:1)

**Clean (saf):**
> "Kulunu geceleyin Mescid-i Haram'dan..." (İsra 17:1)

The word "Muhammed" was added by interpreters - the Arabic only says "abdihi" (His servant).

## Usage / Kullanım

1. **Select a book** from the dropdown (Kuran, İncil, Tevrat, Zebur)
2. **Choose a chapter/sura** from the chapter list
3. **Browse verses** - click on any verse to see word meanings (Quran only)
4. **Search** - enter a keyword and click Search or press Enter
5. **Sort** - for Quran, choose between sura number or revelation order
6. **Export** - save chapters or search results to text files

## Keyboard Shortcuts / Klavye Kısayolları

| Shortcut | Action |
|----------|--------|
| `Ctrl+O` | Open database |
| `Ctrl+,` | Open settings |
| `Ctrl+Q` | Exit |
| `Enter` | Search (in search field) |

## Project Structure / Proje Yapısı

```
HolyBooksExplorer/
├── HolyBooksExplorer.pro    # Qt project file
├── main.cpp                  # Application entry point (with db extraction)
├── mainwindow.h/cpp          # Main window implementation
├── mainwindow.ui             # Main window UI
├── databasemanager.h/cpp     # Database operations
├── settingsdialog.h/cpp      # Settings dialog
├── settingsdialog.ui         # Settings UI
├── searchhighlighter.h/cpp   # Search text highlighter
├── versewidget.h/cpp         # Individual verse display
├── wordanalysiswidget.h/cpp  # Word meanings display
├── resources.qrc             # Qt resources (includes database)
├── Kutsal_Kitaplar.db        # SQLite database
└── translations/             # Translation files
    ├── holybooksexplorer_tr.ts
    └── holybooksexplorer_en.ts
```

---

**Note:** This application is intended for educational and research purposes. The holy texts are presented for study and should be treated with respect.

© 2024 Maren Robotics
