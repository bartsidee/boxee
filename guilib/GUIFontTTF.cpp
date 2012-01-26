/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIFont.h"
#include "GUIFontTTF.h"
#include "GUIFontManager.h"
#include "Texture.h"
#include "GraphicContext.h"
#include "FileSystem/SpecialProtocol.h"
#include "MathUtils.h"
#include "utils/log.h"
#include "AdvancedSettings.h"
#include "WindowingFactory.h"
#include <math.h>

#ifdef HAS_HARFBUZZ_NG
#include <hb-glib.h>
#endif

// stuff for freetype
#ifndef _LINUX
#include "ft2build.h"
#else
#include <ft2build.h>
#endif
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#define USE_RELEASE_LIBS

#ifdef _WIN32
#ifdef _DEBUG
#pragma comment(lib, "../../lib/freetype/objs/win32/vc2010/freetype239MT_D.lib")
#else
#pragma comment(lib, "../../lib/freetype/objs/win32/vc2010/freetype239MT.lib")
#endif
#endif

using namespace std;

#define CHARS_PER_TEXTURE_LINE 20 // number of characters to cache per texture line
#define CHAR_CHUNK    64      // 64 chars allocated at a time (1024 bytes)

int CGUIFontTTFBase::justification_word_weight = 6;   // weight of word spacing over letter spacing when justifying.
                                                  // A larger number means more of the "dead space" is placed between
                                                  // words rather than between letters.

class CFreeTypeLibrary
{
public:
  CFreeTypeLibrary()
  {
    m_library = NULL;
    m_lastFace = NULL;
    m_lastSize = 0;
    m_lastAspect = 0;
    m_lastFilename = "";
  }

  virtual ~CFreeTypeLibrary()
  {
    if (m_lastFace)
      FT_Done_Face(m_lastFace);

    if (m_library)
      FT_Done_FreeType(m_library);
  }

  FT_Face GetFont(const CStdString &filename, float size, float aspect, bool &reloaded);

  void ReleaseFont()
  {
    if (m_lastFace)
      FT_Done_Face(m_lastFace);

    m_lastFace = NULL;
    m_lastFilename = "";
    m_lastSize = 0;
  };

  unsigned int GetDPI() const
  {
    return 72; // default dpi, matches what XPR fonts used to use for sizing
  };

private:
  FT_Library   m_library;
  FT_Face      m_lastFace;
  CStdString   m_lastFilename;
  float        m_lastSize;
  float        m_lastAspect;
};

FT_Face CFreeTypeLibrary::GetFont(const CStdString &filename, float size, float aspect, bool &reloaded)
{
  reloaded = false;

  // don't have it yet - create it
  if (!m_library)
  {
    FT_Init_FreeType(&m_library);
    reloaded = true;
  }

  if (!m_library)
  {
    CLog::Log(LOGERROR, "Unable to initialize freetype library");
    return NULL;
  }

  if (filename != m_lastFilename)
  {
    if (m_lastFace)
    {
      FT_Done_Face(m_lastFace);
      m_lastFace = NULL;
    }

    // ok, now load the font face
    if (FT_New_Face( m_library, _P(filename).c_str(), 0, &m_lastFace))
      return NULL;

    m_lastFilename = filename;
    m_lastSize = 0;
    reloaded = true;
  }

  if (fabs(m_lastSize - size) > 0.1 || fabs(m_lastAspect - aspect) > 0.1)
  {
    unsigned int ydpi = GetDPI();
    unsigned int xdpi = (unsigned int)MathUtils::round_int(ydpi * aspect);

    // we set our screen res currently to 96dpi in both directions (windows default)
    // we cache our characters (for rendering speed) so it's probably
    // not a good idea to allow free scaling of fonts - rather, just
    // scaling to pixel ratio on screen perhaps?
    if (FT_Set_Char_Size( m_lastFace, 0, (int)(size*64 + 0.5f), xdpi, ydpi ))
    {
      //FT_Done_Face(face);
      return NULL;
    }

    m_lastSize = size;
    m_lastAspect = aspect;
    reloaded = true;
  }

  return m_lastFace;
}

CFreeTypeLibrary g_freeTypeLibrary; // our freetype library

CGUIFontTTFBase::CGUIFontTTFBase(const CStdString& strFileName)
{
  m_texture = NULL;
  m_char = NULL;
  m_maxChars = 0;

  m_bTextureLoaded = false;

  m_face = NULL;
  m_faceLoadTime = 0;
#ifdef HAS_HARFBUZZ_NG
  hb_font = NULL;
#endif
  memset(m_charquick, 0, sizeof(m_charquick));
  m_strFileName = strFileName;
  m_referenceCount = 0;
  m_cellBaseLine = m_cellHeight = 0;
  m_numChars = 0;
  m_posX = m_posY = 0;
  m_textureHeight = m_textureWidth = 0;
  m_textureScaleX = m_textureScaleY = 0.0;
  m_ellipsesWidth = m_height = m_aspect = 0.0f;
  m_nTexture = 0;
  m_originX = m_originY = 0.0;
}

CGUIFontTTFBase::~CGUIFontTTFBase(void)
{
  Clear();
}

void CGUIFontTTFBase::AddReference()
{
  m_referenceCount++;
}

void CGUIFontTTFBase::RemoveReference()
{
  // delete this object when it's reference count hits zero
  m_referenceCount--;
  if (!m_referenceCount)
    g_fontManager.FreeFontFile(this);
}


void CGUIFontTTFBase::ClearCharacterCache()
{
  if (m_texture)
  {
    delete(m_texture);
  }

  DeleteHardwareTexture();

  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  m_char = new Character[CHAR_CHUNK];
  memset(m_charquick, 0, sizeof(m_charquick));
  m_numChars = 0;
  m_maxChars = CHAR_CHUNK;
  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -(int)m_cellHeight;
  m_textureHeight = 0;
}

void CGUIFontTTFBase::Clear()
{
  if (m_texture)
    delete(m_texture);

  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  memset(m_charquick, 0, sizeof(m_charquick));
  m_char = NULL;
  m_maxChars = 0;
  m_numChars = 0;
  m_posX = 0;
  m_posY = 0;

#ifdef HAS_HARFBUZZ_NG
  if (hb_font)
  {
    hb_font_destroy (hb_font);
    hb_font = NULL;
  }
#endif

  if (m_face)
  {
    g_freeTypeLibrary.ReleaseFont();
    m_face = NULL;
  }

  m_faceLoadTime = 0;
}

void CGUIFontTTFBase::ReloadFace()
{
  bool reloaded;
  m_face = g_freeTypeLibrary.GetFont(m_strFilename, m_height, m_aspect, reloaded);

  if (!m_face)
    return;

  if (m_faceLoadTime == 0)
    m_faceLoadTime = time(NULL);

#ifdef HAS_HARFBUZZ_NG
  if (reloaded)
  {
    if (hb_font)
    {
      hb_font_destroy (hb_font);
      hb_font = NULL;
    }

    hb_font = hb_ft_font_create (m_face, NULL);
  }
#endif
}

bool CGUIFontTTFBase::Load(const CStdString& strFilename, float height, float aspect, float lineSpacing)
{
  m_strFilename = strFilename;
  m_height = height;
  m_aspect = aspect;

  ReloadFace();

  if (!m_face)
    return false;

  // grab the maximum cell height and width
  unsigned int m_cellWidth = m_face->bbox.xMax - m_face->bbox.xMin;
  m_cellHeight = m_face->bbox.yMax - m_face->bbox.yMin;
  m_cellBaseLine = m_face->bbox.yMax;

  unsigned int ydpi = g_freeTypeLibrary.GetDPI();
  unsigned int xdpi = (unsigned int)MathUtils::round_int(ydpi * aspect);

  m_cellWidth *= (unsigned int)(height * xdpi);
  m_cellWidth /= (72 * m_face->units_per_EM);

  m_cellHeight *= (unsigned int)(height * ydpi);
  m_cellHeight /= (72 * m_face->units_per_EM);

  m_cellBaseLine *= (unsigned int)(height * ydpi);
  m_cellBaseLine /= (72 * m_face->units_per_EM);

  // increment for good measure to give space in our texture
  m_cellWidth++;
  m_cellHeight+=2;
  m_cellBaseLine++;

//  CLog::Log(LOGDEBUG, "%s Scaled size of font %s (%f): width = %i, height = %i, lineheight = %li",
//    __FUNCTION__, strFilename.c_str(), height, m_cellWidth, m_cellHeight, m_face->size->metrics.height / 64);

  if (m_texture)
    delete(m_texture);

  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  m_char = NULL;

  m_maxChars = 0;
  m_numChars = 0;

  m_textureHeight = 0;
  m_textureWidth = ((m_cellHeight * CHARS_PER_TEXTURE_LINE) & ~63) + 64;

  m_textureWidth = CBaseTexture::PadPow2(m_textureWidth);

  if (m_textureWidth > g_Windowing.GetMaxTextureSize())
    m_textureWidth = g_Windowing.GetMaxTextureSize();

  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -(int)m_cellHeight;

  // cache the ellipses width
  Character *ellipse = GetCharacter(L'.');
  if (ellipse) m_ellipsesWidth = ellipse->advance;

  return true;
}

void CGUIFontTTFBase::BuildTextCoordinates(float x, float y, const vecColors &colors, color_t shadowColor, 
                                       const vecText &text, uint32_t alignment, float maxPixelWidth, bool scrolling, FontCoordsIndiced& pData)
{
  // Check if we will really need to truncate or justify the text
  m_originX = x; m_originY = y;

  if ( alignment & XBFONT_TRUNCATED )
  {
    if ( maxPixelWidth <= 0.0f || GetTextWidthInternal(text.begin(), text.end()) <= maxPixelWidth)
      alignment &= ~XBFONT_TRUNCATED;
  }
  else if ( alignment & XBFONT_JUSTIFIED )
  {
    if ( maxPixelWidth <= 0.0f )
      alignment &= ~XBFONT_JUSTIFIED;
  }

  // calculate sizing information
  float startX = 0;
  float startY = (alignment & XBFONT_CENTER_Y) ? -0.5f*(m_cellHeight-2) : 0;  // vertical centering

  if ( alignment & (XBFONT_RIGHT | XBFONT_CENTER_X) )
  {
    // Get the extent of this line
    float w = GetTextWidthInternal( text.begin(), text.end() );

    if ( alignment & XBFONT_TRUNCATED && w > maxPixelWidth )
      w = maxPixelWidth;

    if ( alignment & XBFONT_CENTER_X)
      w *= 0.5f;
    // Offset this line's starting position
    startX -= w;
  }

  float spacePerLetter = 0; // for justification effects
#if 0
  if ( alignment & XBFONT_JUSTIFIED )
  {
    // first compute the size of the text to render in both characters and pixels
    unsigned int lineChars = 0;
    float linePixels = 0;
    for (vecText::const_iterator pos = text.begin(); pos != text.end(); pos++)
    {
      Character *ch = GetCharacter(*pos);
      if (ch)
      { // spaces have multiple times the justification spacing of normal letters
        lineChars += ((*pos & 0xffff) == L' ') ? justification_word_weight : 1;
        linePixels += ch->advance;
      }
    }
    if (lineChars > 1)
      spacePerLetter = (maxPixelWidth - linePixels) / (lineChars - 1);
  }
#endif

  ReloadFace();

#ifdef HAS_HARFBUZZ_NG
  int i = 0;
  wchar_t strW[text.size()];
  for (vecText::const_iterator pos = text.begin(); pos != text.end(); pos++)
  {
    wchar_t letter = (wchar_t)((*pos) & 0xffff);
    strW[i] = letter;
    i++;
  }

  hb_buffer_t *hb_buffer = hb_buffer_create(text.size());
  hb_buffer_set_unicode_funcs(hb_buffer, hb_glib_get_unicode_funcs());
  hb_buffer_add_utf32(hb_buffer, (const uint32_t*) strW, text.size(), 0, text.size());
  hb_buffer_set_direction(hb_buffer, HB_DIRECTION_LTR);

  hb_shape (hb_font, hb_buffer, NULL, 0);
  unsigned int glyph_info_len;
  hb_glyph_info_t *hb_glyph = hb_buffer_get_glyph_infos (hb_buffer, &glyph_info_len);
  hb_glyph_position_t *hb_position = hb_buffer_get_glyph_positions (hb_buffer, &glyph_info_len);

#else /* HAS_HARFBUZZ_NG */
  FT_Vector delta;
  Character* previousCh = NULL;
#endif

  float cursorX = 0; // current position along the line

  for (vecText::const_iterator pos = text.begin(); pos != text.end(); pos++)
  {
    // If starting text on a new line, determine justification effects
    // Get the current letter in the CStdString
    color_t color = (*pos & 0xff0000) >> 16;
    if (color >= colors.size())
      color = 0;
    color = colors[color];

    // grab the next character
    Character *ch = GetCharacter(*pos);
    if (!ch) continue;

    if ( alignment & XBFONT_TRUNCATED )
    {
      // Check if we will be exceeded the max allowed width
      if ( cursorX + ch->advance + 3 * m_ellipsesWidth > maxPixelWidth )
      {
        // Yup. Let's draw the ellipses, then bail
        // Perhaps we should really bail to the next line in this case??
        Character *period = GetCharacter(L'.');
        if (!period)
          break;

        for (int i = 0; i < 3; i++)
        {
          BuildCharacterCoordinates(startX + cursorX, startY, period, color, shadowColor, !scrolling, pData);
          cursorX += period->advance;
        }
        break;
      }
    }
    else if (maxPixelWidth > 0 && cursorX > maxPixelWidth)
      break;  // exceeded max allowed width - stop rendering

#ifdef HAS_HARFBUZZ_NG
    BuildCharacterCoordinates(startX + cursorX + (hb_position->x_offset * (1./64)), startY - (hb_position->y_offset * (1./64)), ch, color, shadowColor, !scrolling, pData);

    if ( alignment & XBFONT_JUSTIFIED )
    {
      if ((*pos & 0xffff) == L' ')
        cursorX +=  (hb_position->x_advance * (1./64)) + spacePerLetter * justification_word_weight;
      else
        cursorX += (hb_position->x_advance * (1./64)) + spacePerLetter;
    }
    else
      cursorX +=  hb_position->x_advance * (1./64);

    hb_glyph++;
    hb_position++;
#else
    if (previousCh)
    {
      FT_Get_Kerning(m_face, previousCh->glyphIndex, ch->glyphIndex,
          FT_KERNING_DEFAULT, &delta);
      cursorX += (float) (delta.x / 64);
    }

    BuildCharacterCoordinates(startX + cursorX, startY, ch, color, shadowColor, !scrolling, pData);
    if ( alignment & XBFONT_JUSTIFIED )
    {
      if ((*pos & 0xffff) == L' ')
        cursorX += ch->advance + spacePerLetter * justification_word_weight;
      else
        cursorX += ch->advance + spacePerLetter;
    }
    else
      cursorX += ch->advance;

    previousCh = ch;
#endif
  }

#ifdef HAS_HARFBUZZ_NG
  hb_buffer_destroy(hb_buffer);
#endif
}

// this routine assumes a single line (i.e. it was called from GUITextLayout)
float CGUIFontTTFBase::GetTextWidthInternal(vecText::const_iterator start, vecText::const_iterator end)
{
  float width = 0;
  while (start != end)
  {
    Character *c = GetCharacter(*start++);
    if (c) width += c->advance;
  }
  return width;
}

float CGUIFontTTFBase::GetCharWidthInternal(character_t ch)
{
  Character *c = GetCharacter(ch);
  if (c) return c->advance;
  return 0;
}

float CGUIFontTTFBase::GetTextHeight(float lineSpacing, int numLines) const
{
  return (float)(numLines - 1) * GetLineHeight(lineSpacing) + (m_cellHeight - 2); // -2 as we increment this for space in our texture
}

float CGUIFontTTFBase::GetLineHeight(float lineSpacing) const
{
  if (m_face)
//    return lineSpacing * m_face->size->metrics.height / 64.0f;
      return lineSpacing * m_cellHeight;

    return 0.0f;
}

CGUIFontTTFBase::Character* CGUIFontTTFBase::GetCharacter(character_t chr)
{
  wchar_t letter = (wchar_t)(chr & 0xffff);
  character_t style = (chr & 0x3000000) >> 24;

  // ignore linebreaks
  if (letter == L'\r')
    return NULL;

  // quick access to ascii chars
  if (letter < 255)
  {
    character_t ch = (style << 8) | letter;
    if (m_charquick[ch])
      return m_charquick[ch];
  }

  // letters are stored based on style and letter
  character_t ch = (style << 16) | letter;

  int low = 0;
  int high = m_numChars - 1;
  int mid;
  while (low <= high)
  {
    mid = (low + high) >> 1;
    if (ch > m_char[mid].letterAndStyle)
      low = mid + 1;
    else if (ch < m_char[mid].letterAndStyle)
      high = mid - 1;
    else
      return &m_char[mid];
  }
  // if we get to here, then low is where we should insert the new character

  // increase the size of the buffer if we need it
  if (m_numChars >= m_maxChars)
  { // need to increase the size of the buffer
    Character *newTable = new Character[m_maxChars + CHAR_CHUNK];
    if (m_char)
    {
      memcpy(newTable, m_char, low * sizeof(Character));
      memcpy(newTable + low + 1, m_char + low, (m_numChars - low) * sizeof(Character));
      delete[] m_char;
    }
    m_char = newTable;
    m_maxChars += CHAR_CHUNK;

  }
  else
  { // just move the data along as necessary
    memmove(m_char + low + 1, m_char + low, (m_numChars - low) * sizeof(Character));
  }
  // render the character to our texture
  // must End() as we can't render text to our texture during a Begin(), End() block
  if (!CacheCharacter(letter, style, m_char + low))
  { // unable to cache character - try clearing them all out and starting over
    CLog::Log(LOGDEBUG, "GUIFontTTF::GetCharacter: Unable to cache character.  Clearing character cache of %i characters", m_numChars);
    ClearCharacterCache();
    low = 0;
    if (!CacheCharacter(letter, style, m_char + low))
    {
      CLog::Log(LOGERROR, "GUIFontTTF::GetCharacter: Unable to cache character (out of memory?)");
      return NULL;
    }
  }

  // fixup quick access
  memset(m_charquick, 0, sizeof(m_charquick));
  for(int i=0;i<m_numChars;i++)
  {
    if ((m_char[i].letterAndStyle & 0xffff) < 255)
    {
      character_t ch = ((m_char[i].letterAndStyle & 0xffff0000) >> 8) | (m_char[i].letterAndStyle & 0xff);
      m_charquick[ch] = m_char+i;
    }
  }

  return m_char + low;
}

bool CGUIFontTTFBase::CacheCharacter(wchar_t letter, uint32_t style, Character *ch)
{
  ReloadFace();

  int glyph_index = FT_Get_Char_Index( m_face, letter );

  FT_Glyph glyph = NULL;
  if (FT_Load_Glyph( m_face, glyph_index, FT_LOAD_DEFAULT  ))
  {
    CLog::Log(LOGDEBUG, "%s Failed to load glyph %x", __FUNCTION__, letter);
    return false;
  }
  // make bold if applicable
  if (style & FONT_STYLE_BOLD)
    EmboldenGlyph(m_face->glyph);
  // and italics if applicable
  if (style & FONT_STYLE_ITALICS)
    ObliqueGlyph(m_face->glyph);
  // grab the glyph
  if (FT_Get_Glyph(m_face->glyph, &glyph))
  {
    CLog::Log(LOGDEBUG, "%s Failed to get glyph %x", __FUNCTION__, letter);
    return false;
  }
  // render the glyph
  if (FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, 1))
  {
    CLog::Log(LOGDEBUG, "%s Failed to render glyph %x to a bitmap", __FUNCTION__, letter);
    return false;
  }
  FT_BitmapGlyph bitGlyph = (FT_BitmapGlyph)glyph;
  FT_Bitmap bitmap = bitGlyph->bitmap;
  if (bitGlyph->left < 0)
    m_posX += -bitGlyph->left;

  // check we have enough room for the character
  if (m_posX + bitGlyph->left + bitmap.width > (int)m_textureWidth)
  { // no space - gotta drop to the next line (which means creating a new texture and copying it across)
    m_posX = 0;
    m_posY += m_cellHeight;
    if (bitGlyph->left < 0)
      m_posX += -bitGlyph->left;

    if(m_posY + m_cellHeight >= m_textureHeight)
    {
      // create the new larger texture
      unsigned int newHeight = m_posY + m_cellHeight;
      // check for max height
      if (newHeight > g_Windowing.GetMaxTextureSize())
      {
        CLog::Log(LOGDEBUG, "GUIFontTTF::CacheCharacter: New cache texture is too large (%u > %u pixels long)", newHeight, g_Windowing.GetMaxTextureSize());
        FT_Done_Glyph(glyph);
        return false;
      }
            
      CBaseTexture* newTexture = NULL;
      newTexture = ReallocTexture(newHeight);
      if(newTexture == NULL)
      {
        FT_Done_Glyph(glyph);
        CLog::Log(LOGDEBUG, "GUIFontTTF::CacheCharacter: Failed to allocate new texture of height %u", newHeight);
        return false;
      }
      m_texture = newTexture;
    }
  }

  if(m_texture == NULL)
  {
    CLog::Log(LOGDEBUG, "GUIFontTTF::CacheCharacter: no texture to cache character to");
    return false;
  }

  // set the character in our table
  ch->letterAndStyle = (style << 16) | letter;
  ch->offsetX = (short)bitGlyph->left;
  ch->offsetY = (short)max((short)m_cellBaseLine - bitGlyph->top, 0);
  ch->left = (float)m_posX + ch->offsetX;
  ch->top = (float)m_posY + ch->offsetY;
  ch->right = ch->left + bitmap.width;
  ch->bottom = ch->top + bitmap.rows;
  ch->advance = (float)MathUtils::round_int( (float)m_face->glyph->advance.x / 64 );
  ch->glyphIndex = glyph_index;

  // we need only render if we actually have some pixels
  if (bitmap.width * bitmap.rows)
  {
    CopyCharToTexture(bitGlyph, ch);
  }
  m_posX += (unsigned short)max(ch->right - ch->left + ch->offsetX, ch->advance + 1);
  m_numChars++;

  if(m_textureWidth == 0 || m_textureHeight == 0)
    return false;

  m_textureScaleX = 1.0f / (m_textureWidth);
  m_textureScaleY = 1.0f / (m_textureHeight - 1);

  // free the glyph
  FT_Done_Glyph(glyph);

  return true;
}

void CGUIFontTTFBase::BuildCharacterCoordinates(float posX, float posY, const Character *ch, color_t color, 
                                                color_t shadowColor, bool roundX, FontCoordsIndiced& pData)
{
  /*
  static int count = 0;
  CLog::Log(LOGINFO, "CGUIFontTTFBase::BuildCharacterCoordinates %d", count++);
  */
  
  // actual image width isn't same as the character width as that is
  // just baseline width and height should include the descent
  const float width = ch->right - ch->left;
  const float height = ch->bottom - ch->top;

  // posX and posY are relative to our origin, and the textcell is offset
  // from our (posX, posY).  Plus, these are unscaled quantities compared to the underlying GUI resolution
  CRect vertex((posX + ch->offsetX),
               (posY + ch->offsetY),
               (posX + ch->offsetX + width),
               (posY + ch->offsetY + height));
  vertex += CPoint(0.0 -  m_originX, 0 - m_originY);

  CRect texture(ch->left - 0.5f, ch->top - 0.5f, ch->right + 0.5f, ch->bottom + 0.5f);
  //CRect texture(ch->left, ch->top, ch->right, ch->bottom);

  // transform our positions - note, no scaling due to GUI calibration/resolution occurs
  float x[4];

  x[0] = vertex.x1;
  x[1] = vertex.x2;
  x[2] = vertex.x2;
  x[3] = vertex.x1;

  float y1 =  vertex.y1;
  float y2 =  vertex.y1;
  float y3 =  vertex.y2;
  float y4 =  vertex.y2;

  float z1 = 0;
  float z2 = 0;
  float z3 = 0;
  float z4 = 0;

  // tex coords converted to 0..1 range
  float tl = texture.x1 * m_textureScaleX;
  float tr = texture.x2 * m_textureScaleX;
  float tt = texture.y1 * m_textureScaleY;
  float tb = texture.y2 * m_textureScaleY;

  FontCoords coords;
  FontVertex* v = coords.m_pCoords;

  for(int i = 0; i < 4; i++)
  {
    v[i].r = GET_R(color);
    v[i].g = GET_G(color);
    v[i].b = GET_B(color);
    v[i].a = GET_A(color);
    v[i].rs = GET_R(shadowColor);
    v[i].gs = GET_G(shadowColor);
    v[i].bs = GET_B(shadowColor);
    v[i].as = GET_A(shadowColor);
  }

  v[0].u1 = tl;
  v[0].v1 = tt;
  v[0].x = x[0];
  v[0].y = y1;
  v[0].z = z1;

  v[1].u1 = tr;
  v[1].v1 = tt;
  v[1].x = x[1];
  v[1].y = y2;
  v[1].z = z2;

  v[2].u1 = tr;
  v[2].v1 = tb;
  v[2].x = x[2];
  v[2].y = y3;
  v[2].z = z3;

  v[3].u1 = tl;
  v[3].v1 = tb;
  v[3].x = x[3];
  v[3].y = y4;
  v[3].z = z4;

  unsigned int indices[6] = 
  {
    0, 1, 2,
    2, 3, 0,
  };

  pData.m_pCoord.push_back(coords);
  unsigned int pos = pData.m_pCoord.size() - 1;
  // push indices
  for(int i = 0; i < 6; i++)
    pData.m_nIndices.push_back(indices[i] + 4 * pos);
}

// Oblique code - original taken from freetype2 (ftsynth.c)
void CGUIFontTTFBase::ObliqueGlyph(FT_GlyphSlot slot)
{
  /* only oblique outline glyphs */
  if ( slot->format != FT_GLYPH_FORMAT_OUTLINE )
    return;

  /* we don't touch the advance width */

  /* For italic, simply apply a shear transform, with an angle */
  /* of about 12 degrees.                                      */

  FT_Matrix    transform;
  transform.xx = 0x10000L;
  transform.yx = 0x00000L;

  transform.xy = 0x06000L;
  transform.yy = 0x10000L;

  FT_Outline_Transform( &slot->outline, &transform );
}


// Embolden code - original taken from freetype2 (ftsynth.c)
void CGUIFontTTFBase::EmboldenGlyph(FT_GlyphSlot slot)
{
  if ( slot->format != FT_GLYPH_FORMAT_OUTLINE )
    return;

  /* some reasonable strength */
  FT_Pos strength = FT_MulFix( m_face->units_per_EM,
                    m_face->size->metrics.y_scale ) / 28;

  FT_BBox bbox_before, bbox_after;
  FT_Outline_Get_CBox( &slot->outline, &bbox_before );
  FT_Outline_Embolden( &slot->outline, strength );  // ignore error
  FT_Outline_Get_CBox( &slot->outline, &bbox_after );

  FT_Pos dx = bbox_after.xMax - bbox_before.xMax;
  FT_Pos dy = bbox_after.yMax - bbox_before.yMax;

  if ( slot->advance.x )
    slot->advance.x += dx;

  if ( slot->advance.y )
    slot->advance.y += dy;

  slot->metrics.width        += dx;
  slot->metrics.height       += dy;
  slot->metrics.horiBearingY += dy;
  slot->metrics.horiAdvance  += dx;
  slot->metrics.vertBearingX -= dx / 2;
  slot->metrics.vertBearingY += dy;
  slot->metrics.vertAdvance  += dy;
}


