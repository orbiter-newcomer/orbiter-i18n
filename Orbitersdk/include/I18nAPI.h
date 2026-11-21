// ==============================================================
// I18nAPI.h - Chinese localisation layout / dialog helper API
// (project extension).
//
// Included by Orbitersdk/include/OrbiterAPI.h: the official SDK header
// only grows by the include line, all project API declarations live here.
// ==============================================================

#ifndef __I18N_API_H
#define __I18N_API_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
/**
 * \brief Queries runtime layout data of the Chinese localisation.
 * \param section layout section name (e.g. "mfd.landing")
 * \param key entry name inside the section (e.g. "navRowX")
 * \param def value returned when the entry is not present
 * \return parsed layout value (integer / floating point / string)
 * \note Layout values live in i18n/layout/zh_CN/layout.ini and can be edited
 *   without rebuilding the program. Missing files/entries always fall back to
 *   the passed default, so a plugin behaves identically when no layout data is
 *   installed. English mode does not load layout data, i.e. defaults apply.
 *   The layout system is intended for Chinese localisation metrics (offsets,
 *   font sizes, ...), not for general plugin configuration.
 */
OAPIFUNC int oapiGetLayoutInt (const char *section, const char *key, int def);
OAPIFUNC double oapiGetLayoutFloat (const char *section, const char *key, double def);
OAPIFUNC const char *oapiGetLayoutString (const char *section, const char *key, const char *def);

/**
 * \brief Fits a dialog control to its translated Chinese text.
 * \param hDlg dialog window
 * \param ctrlId control identifier
 * \param padX horizontal padding added to the measured text width
 * \return 1 if the control was adjusted, 0 otherwise (English mode, missing
 *   control, or the original size already fits)
 * \note Implements the "official layout + offset" rule for dialogs: the RC
 *   resources keep the official English layout; in Chinese mode the control
 *   text is translated (oapiTr) and, if the Chinese text does not fit, the
 *   control is widened while keeping its left edge and vertical position.
 *   The official layout stays authoritative in English mode.
 */
OAPIFUNC int oapiZhFitControlText (HWND hDlg, int ctrlId, int padX);

/**
 * \brief Fits and repacks a horizontal row of dialog controls (buttons).
 * \param hDlg dialog window
 * \param ctrlIds control identifiers in left-to-right order
 * \param count number of identifiers
 * \param padX horizontal padding added to each measured text width
 * \param gap gap between adjacent controls
 * \param startX left edge of the row; if negative, the current left edge of the
 *   first control is used
 * \return number of controls that were adjusted
 * \note Same "official layout + offset" rule as oapiZhFitControlText, but the
 *   controls are packed left-to-right so that wider Chinese button texts do not
 *   overlap. English mode does not touch the layout.
 */
OAPIFUNC int oapiZhFitControlRow (HWND hDlg, const int *ctrlIds, int count, int padX, int gap, int startX);

/**
 * \brief Grows a dialog control vertically to fit its translated Chinese text.
 * \param hDlg dialog window
 * \param ctrlId control identifier
 * \param padY vertical padding added to the measured text height
 * \return 1 if the control was adjusted, 0 otherwise
 * \note Same "official layout + offset" rule as oapiZhFitControlText, but only
 *   the height is adapted (width and position of the control stay unchanged).
 *   Useful for button rows whose horizontal packing must not change.
 */
OAPIFUNC int oapiZhFitControlHeight (HWND hDlg, int ctrlId, int padY);

/**
 * \brief Moves dialog controls by a pixel offset (Chinese layout adjustment).
 * \param hDlg dialog window
 * \param ctrlIds control identifiers
 * \param count number of identifiers
 * \param dx horizontal offset in pixels
 * \param dy vertical offset in pixels
 * \return number of controls moved
 * \note Implements the "official layout + offset" rule for controls whose
 *   translated texts overlap neighbouring controls: the official rc position
 *   is kept as the base and only a small offset from layout.ini is applied.
 *   English mode never moves anything.
 */
OAPIFUNC int oapiZhOffsetControls (HWND hDlg, const int *ctrlIds, int count, int dx, int dy);

/**
 * \brief Moves one dialog control by a pixel offset (Chinese layout adjustment).
 * \param hCtrl control window (may be obtained with GetDlgItem or by enumerating
 *   the dialog's child windows, e.g. for controls that share a static ID)
 * \param dx horizontal offset in pixels
 * \param dy vertical offset in pixels
 * \return 1 if the control was moved, 0 otherwise
 * \note Same "official layout + offset" rule as oapiZhOffsetControls, for
 *   individual controls. English mode never moves anything.
 */
OAPIFUNC int oapiZhOffsetWindow (HWND hCtrl, int dx, int dy);

/**
 * \brief Lays out all buttons of a dialog for the Chinese UI in one call.
 * \param hDlg dialog window
 * \param padY vertical padding added to the measured text height
 * \param yOffset downward offset (pixels) applied to every button
 * \return number of buttons that were adjusted
 * \note Applies the "official layout + offset" rule to a complete dialog:
 *   every push button is translated (oapiTr), grown vertically when the Chinese
 *   text does not fit the official height, and then moved down by yOffset.
 *   Width and horizontal position of the official layout are never changed, so
 *   button rows cannot overlap horizontally. Both values normally come from
 *   layout.ini ([dialog.*] sections), so no per-control code is needed.
 *   English mode does not touch the layout.
 */
OAPIFUNC int oapiZhLayoutDialogButtons (HWND hDlg, int padY, int yOffset);

/**
 * \brief Fits every push button of a dialog to its translated Chinese text.
 * \param hDlg dialog window
 * \param padX horizontal padding added to each measured text width
 * \param padY vertical padding added to each measured text height
 * \return number of buttons that were adjusted
 * \note Convenience wrapper for dialogs with many buttons (e.g. vessel editor
 *   pages): all push buttons are translated and, where the Chinese text does
 *   not fit the official rect, enlarged. Buttons stacked in one column are
 *   repacked so that taller buttons do not overlap; standalone buttons grow
 *   around their centre. Offsets come from layout.ini ([dialog.*] sections).
 *   English mode does not touch the layout at all.
 */
OAPIFUNC int oapiZhFitDialogButtons (HWND hDlg, int padX, int padY);

#endif // !__I18N_API_H
