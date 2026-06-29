#pragma once
#include "Config.h"
#include "SimulationConfig.h"
#include "common/String.h"

inline ByteString VersionInfo()
{
	ByteStringBuilder sb;
	sb << DISPLAY_VERSION[0] << "." << DISPLAY_VERSION[1];
	if constexpr (!SNAPSHOT)
	{
		sb << "." << APP_VERSION.build;
	}
	sb << " " << IDENT;
	if constexpr (MOD)
	{
		sb << " MOD " << MOD_ID << " UPSTREAM " << UPSTREAM_VERSION.build;
	}
	if constexpr (SNAPSHOT)
	{
		sb << " SNAPSHOT " << APP_VERSION.build;
	}
	if constexpr (LUACONSOLE)
	{
		sb << " LUACONSOLE";
	}
	if constexpr (NOHTTP)
	{
		sb << " NOHTTP";
	}
	else if constexpr (ENFORCE_HTTPS)
	{
		sb << " HTTPS";
	}
	if constexpr (DEBUG)
	{
		sb << " DEBUG";
	}
	return sb.Build();
}

inline ByteString IntroText()
{
	ByteStringBuilder sb;
	sb << "\bl\bU" << APPNAME << "\bU - Версия " << DISPLAY_VERSION[0] << "." << DISPLAY_VERSION[1] << " - https://powdertoy.co.uk, irc.libera.chat #powder, https://tpt.io/discord\n"
	      "\n"
	      "\n"
	      "\bgНажмите \bo'F1'\bg, чтобы показать или скрыть этот текст.\n"
	      "\n"
	      "\bgЧтобы выбрать материал, наведите курсор на значок справа — откроется группа элементов.\n"
	      "Выберите материал в меню \boЛКМ/ПКМ\bg.\n"
	      "Рисуйте линии, перетаскивая \boЛКМ/ПКМ\bg по области рисования.\n"
	      "\n"
	      "Колёсико мыши или \bo'['\bg и \bo']'\bg меняют размер кисти. \boTab\bg — форма кисти.\n"
	      "\boСредняя кнопка\bg или \boAlt+клик\bg — взять образец частиц.\n"
	      "\boCtrl+C/V/X\bg — копировать, вставить и вырезать.\n"
	      "При вставке: \bo'R'\bg — поворот, \boShift+R\bg и \boShift+Ctrl+R\bg — отражение по вертикали и горизонтали.\n"
	      "\boShift+перетаскивание\bg — прямые линии. \boShift+Alt+перетаскивание\bg — горизонталь, вертикаль и диагональ.\n"
	      "\boCtrl+перетаскивание\bg — залитые прямоугольники. \boCtrl+Alt+перетаскивание\bg — квадраты. \boCtrl+Shift+клик\bg — заливка замкнутой области.\n"
	      "\n"
	      "\boПробел\bg — пауза симуляции. \bo'F'\bg — один кадр вперёд, \bo'F5'\bg — перезагрузка симуляции.\n"
	      "\boCtrl+Z\bg — отмена, \boCtrl+Y\bg или \boCtrl+Shift+Z\bg — повтор.\n"
	      " \bo'S'\bg — сохранить область как штамп. \bo'L'\bg — последний штамп, \bo'K'\bg — библиотека штампов.\n"
	      "\n"
	      "\bo0-9\bg — режим отображения.\n"
	      "\bo'H'\bg — HUD вкл/выкл. \bo'D'\bg — отладка в HUD.\n"
	      "\bo'Z'\bg — инструмент масштаба. Клик закрепляет окно лупы. Колёсико меняет силу масштаба.\n"
	      "\boCtrl+F\bg — подсветить выбранный элемент на экране.\n"
	      "\n";
	if constexpr (BETA)
	{
		sb << "\brЭто БЕТА: нельзя публиковать сохранения и открывать локальные сохранения и штампы из неё в старых версиях.\n"
		      "\brДля публикации сохранений используйте релизную версию.\n";
	}
	else
	{
		sb << "\bgДля онлайн-функций, в том числе сохранения, зарегистрируйтесь: \br" << SERVER << "/Register.html\n";
	}
	sb << "\n\bt" << VersionInfo();
	return sb.Build();
}
