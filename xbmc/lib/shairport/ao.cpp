#include <stdlib.h>
#include <ao/ao.h>

#include "ApplicationMessenger.h"
#include "FilePipe.h"
#include "Application.h"
#include "cores/paplayer/BXAcodec.h"
#include "MusicInfoTag.h"

char* ao_get_option(ao_option *options, const char* key);

struct ao_device_boxee
{
  XFILE::CFilePipe *pipe;
};

void ao_initialize(void)
{
}

int ao_play(ao_device *device, char *output_samples, uint32_t num_bytes)
{
  if (!device)
    return 0;

  if (num_bytes && g_application.m_pPlayer)
    g_application.m_pPlayer->SetCaching(false);

  ao_device_boxee* device_boxee = (ao_device_boxee*) device;

#define NUM_OF_BYTES 64

  unsigned int sentBytes = 0;
  unsigned char buf[NUM_OF_BYTES];
  while (sentBytes < num_bytes)
  {
    int n = (num_bytes - sentBytes < NUM_OF_BYTES ? num_bytes - sentBytes : NUM_OF_BYTES);
    memcpy(buf, (char*) output_samples + sentBytes, n);

    if (device_boxee->pipe->Write(buf, n) == 0)
      return 0;

    sentBytes += n;
  }

  return 1;
}

int ao_default_driver_id(void)
{
  return 0;
}

ao_device* ao_open_live(int driver_id, ao_sample_format *format,
    ao_option *option)
{
  ao_device_boxee* device = new ao_device_boxee();

  device->pipe = new XFILE::CFilePipe;
  device->pipe->OpenForWrite(XFILE::PipesManager::GetInstance().GetUniquePipeName());
  device->pipe->SetOpenThreashold(300);

  BXA_FmtHeader header;
  strncpy(header.fourcc, "BXA ", 4);
  header.type = BXA_PACKET_TYPE_FMT;
  header.bitsPerSample = format->bits;
  header.channels = format->channels;
  header.sampleRate = format->rate;
  header.durationMs = 0;

  if (device->pipe->Write(&header, sizeof(header)) == 0)
    return 0;

  ThreadMessage tMsg ( TMSG_MEDIA_STOP );
  g_application.getApplicationMessenger().SendMessage(tMsg, true);

  CFileItem item;
  item.m_strPath = device->pipe->GetName();
  item.SetContentType("audio/x-boxee-pcm");
  item.SetProperty("isradio", true);
  item.SetProperty("no-skip", true);
  item.SetProperty("no-pause", true);

  if (ao_get_option(option, "artist"))
    item.GetMusicInfoTag()->SetArtist(ao_get_option(option, "artist"));

  if (ao_get_option(option, "album"))
    item.GetMusicInfoTag()->SetAlbum(ao_get_option(option, "album"));

  if (ao_get_option(option, "name"))
    item.GetMusicInfoTag()->SetTitle(ao_get_option(option, "name"));

  g_application.getApplicationMessenger().PlayFile(item);

  ThreadMessage tMsg2 ( TMSG_GUI_ACTIVATE_WINDOW, WINDOW_VISUALISATION, 0 );
  g_application.getApplicationMessenger().SendMessage(tMsg2, true);

  return (ao_device*) device;
}

int ao_close(ao_device *device)
{
  ao_device_boxee* device_boxee = (ao_device_boxee*) device;
  device_boxee->pipe->SetEof();
  device_boxee->pipe->Close();
  delete device_boxee->pipe;

  if (g_application.IsPlaying() && g_application.GetCurrentPlayer() == EPC_PAPLAYER)
  {
    ThreadMessage tMsg ( TMSG_MEDIA_STOP );
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
  }

  delete device_boxee;

  return 0;
}

/* -- Device Setup/Playback/Teardown -- */
int ao_append_option(ao_option **options, const char *key, const char *value)
{
  ao_option *op, *list;

  op = (ao_option*) calloc(1,sizeof(ao_option));
  if (op == NULL) return 0;

  op->key = strdup(key);
  op->value = strdup(value?value:"");
  op->next = NULL;

  if ((list = *options) != NULL) {
    list = *options;
    while (list->next != NULL) list = list->next;
    list->next = op;
  } else {
    *options = op;
  }

  return 1;
}

void ao_free_options(ao_option *options)
{
  ao_option *rest;

  while (options != NULL) {
    rest = options->next;
    free(options->key);
    free(options->value);
    free(options);
    options = rest;
  }
}

char* ao_get_option(ao_option *options, const char* key)
{

  while (options != NULL)
  {
    if (strcmp(options->key, key) == 0)
      return options->value;
    options = options->next;
  }

  return NULL;
}
