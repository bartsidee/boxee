#ifndef STACKED_MOVIE_BACKSEEK_HANDLER_H
#define STACKED_MOVIE_BACKSEEK_HANDLER_H


class CStackedMovieBackSeekHandler : public IGUIThreadTask
{
  __int64 m_seek;
public:
  CStackedMovieBackSeekHandler(__int64 seek) : m_seek(seek) {};
  virtual void DoWork()
  {
    g_application.StackedMovieBackSeekHandler(m_seek);
  }

};

#endif

