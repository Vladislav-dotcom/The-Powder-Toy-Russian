# The Powder Toy — русская версия

Форк: https://github.com/Vladislav-dotcom/The-Powder-Toy-Russian

## Подход

- Нет i18n-фреймворка: строки захардкожены в C++ (UTF-8).
- Коды элементов/инструментов (`WATR`, `HEAT`, `ADD`) **не переводятся**.
- Переводятся: `Description`, UI, меню, стены, F1, HUD.
- `GameView.cpp` — HUD (Темп., Давление, Вр.1/Вр.2), кнопки нижней панели, отладочный HUD.
- Кириллица в шрифте `resources/font.bz2` уже есть.
- Таблички: `GameSave.cpp` — `CleanString` с `ascii=false` для кириллицы в saves.
- `ServerSaveActivity.cpp` — диалог сохранения на сервер, правила загрузки, «О публикации».

## Сборка (Windows, release static)

```powershell
meson setup -Dbuildtype=release -Dstatic=prebuilt -Db_vscrt=static_from_buildtype `
  -Dapp_name="The Powder Toy — Русская версия" build-release
cd build-release
meson compile
```

Exe: `build-release\powder.exe`

**Требуется:** Visual Studio (Desktop C++) или MSYS2 UCRT64 с g++. На этой машине компилятор не установлен — сборка не выполнена.

Локальные скрипты перевода (не в git): `scripts/element_translations.py`, `scripts/apply_element_translations.py`

## Переведённые GUI-файлы

- `src/gui/options/OptionsView.cpp` — окно настроек
- `src/gui/game/QuickOptions.cpp` — быстрые опции (P/G/D/N/A/C)
- `src/gui/game/IntroText.h` — текст F1
- `src/gui/game/tool/SignTool.cpp`, `GOLTool.cpp`, `PropertyTool.cpp` — окна инструментов
- `src/simulation/Sign.cpp` — «Пусто»/«пусто» на табличках
- `src/gui/elementsearch/ElementSearchActivity.cpp` — поиск элементов
- `src/gui/update/UpdateActivity.cpp` — автообновление
- `src/gui/colourpicker/ColourPickerActivity.cpp` — «Готово»
- `src/gui/render/RenderView.cpp` — подсказки режимов отрисовки
- `src/gui/credits/Credits.cpp` — заголовки, «Закрыть»
- `src/gui/game/GameController.cpp` — диалоги, SetInfoTip, уведомления об обновлении
- `src/gui/search/SearchView.cpp`, `SearchController.cpp` — поиск сохранений, F1-помощь
- `src/gui/login/LoginView.cpp`, `LoginModel.cpp` — вход на сервер
- `src/gui/localbrowser/LocalBrowserView.cpp`, `LocalBrowserController.cpp` — локальные штампы
- `src/gui/save/LocalSaveActivity.cpp` — сохранение на диск
- `src/gui/tags/TagsView.cpp`, `TagsModel.cpp` — управление тегами
- `src/gui/dialogues/` — ErrorMessage, ConfirmPrompt, InformationMessage, TextPrompt, SaveIDMessage
- `src/gui/preview/PreviewView.cpp`, `PreviewModel.cpp` — просмотр сохранения, комментарии, жалобы
- `src/gui/profile/ProfileActivity.cpp` — профиль пользователя
- `src/gui/filebrowser/FileBrowserActivity.cpp` — локальный обзор сохранений

## Git

- `origin` — upstream The-Powder-Toy/The-Powder-Toy
- `russian` — Vladislav-dotcom/The-Powder-Toy-Russian
