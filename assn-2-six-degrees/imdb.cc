
#include "imdb.h"

#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace std;

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

//general struct to hold data for comparison
typedef struct Info{
  const void* info;
  const void* base;
} Info;

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

// you should be implementing these two methods right here...

void getActor(const void *actorFile, int actor_offset,
              const char** actor_info){
  //having the base address of the actorFile and the offset at which
  //the actorData is stored, through @param 'actor_info' the 
  //actor's name is returned
  const char* actor_addr = (char*)actorFile + actor_offset;
  *actor_info = actor_addr;
}

int cmpActors(const void* p_act1, const void* p_act2){

  //p_act1 is (Info*) and p_act2 is (int*)
  //compute the offset of the actor in the file to find actorData
  //then compare actor names
  Info *actor = (Info*)p_act1;

  const char *name1 = *(char**)actor->info;
  const void *base_addr = actor->base;
  const int offset_actor2 = *(int*)p_act2;
  const char *name2;

  getActor(base_addr, offset_actor2, &name2);

  return strcmp(name1, name2);
}

void getMovie(const void *movieFile, int movie_offset, film *movie_info){
  
  //having the base address and the offset of the movie in the file
  //getMovie returns through @param 'movie_info' the movie found at
  //the specified offset
  const char *movie_addr = (char*)movieFile + movie_offset;

  int len_movie_name = strlen(movie_addr);
  movie_info->title = movie_addr;
  movie_addr += len_movie_name + 1;

  unsigned char delta_year = *movie_addr;
  movie_info->year = delta_year + 1900;
}

void populateMovieList(const void *movieFile, const void *address,
                       short num_movies, vector<film>& films){
  //for each offset
  for(int i = 0; i < num_movies; ++i){

    //search into the movie file for the name and year of the movie
    //year of movie is delta from 1900(1 byte)
    int movie_offset = *((int*)address + i);
    //std::cout << movie_offset <<std::endl;
    film film_info;
    getMovie(movieFile, movie_offset, &film_info);
    films.push_back(film_info);
  }
}

bool imdb::getCredits(const string& player, vector<film>& films) const {
  //actorFile stores data like a void* array
  int num_actors = *(int*)actorFile;
  Info actor;
  const char *name = player.c_str();
  actor.info = &name; 
  actor.base = actorFile;
  //binary search through the offsets at which the actor data is stored
  void* actor_addr_offset = bsearch(&actor, (int*)actorFile + 1, num_actors,
                                sizeof(int), cmpActors);
  //returns address of actor offset
  //if match found
  if(actor_addr_offset == NULL){
    films.clear();
    return false;
  }
  //move the pointer past actor name + '\0' (and another '\0' if is the case)
  int actor_offset = *(int*)actor_addr_offset;
  const char* actor_addr = (char*)actorFile + actor_offset;
  actor_offset = 0;

  actor_offset += player.size() + 1;
  if(player.size() % 2 == 0) ++actor_offset;

  //remember number of movies and  move the pointer
  //2 bytes more if it's the case

  short num_movies;
  memcpy(&num_movies, actor_addr + actor_offset, sizeof(short));
  actor_offset += 2;

  if(actor_offset % 4 != 0) actor_addr += 2;

  //load the movies in the 'films' vector
  populateMovieList(movieFile, actor_addr + actor_offset, num_movies, films);
  
  return true;
}


//function to compare two films
//uses the offset provided at the header of the movieFile

int cmpMovies(const void *m_ptr1, const void *m_ptr2){
  //m_ptr1 (Info*), m_ptr2 (int*)
  //given the offset of the second movie, calculates the position,
  //retrieves the data, and does the movie comparison
  Info *movie = (Info*)m_ptr1;
  const film film1 = *(film*)movie->info;
  const void *movieFile = movie->base;
  int movie_offset = *(int*)m_ptr2;

  film film2;
  getMovie(movieFile, movie_offset, &film2);

  if(film1 < film2) return -1;
  else if(film1 == film2) return 0;
  return 1;
}

void populateActorList(const void *actorFile, const void *address,
                       short num_actors, vector<string>& players){
  
  //given the actorFile and the address to the beginning of the
  //actor offset array, it returns through @param players an vector
  //containing actor names
  for(int i = 0; i < num_actors; ++i){
    int actor_offset = *((int*)address + i);
    const char *actor_info;
    getActor(actorFile, actor_offset, &actor_info);
    string s(actor_info);
    players.push_back(s);
  }
}

bool imdb::getCast(const film& movie, vector<string>& players) const {
  //movieFile stores data like a void* array
  int num_movies = *(int*)movieFile;

  //we need to store the base address for use in the comparison function
  Info movie_info;
  movie_info.base = movieFile;
  movie_info.info = &movie;

  //search for movie
  void *movie_addr_offset = bsearch(&movie_info, (int*)movieFile + 1,
                                    num_movies, sizeof(int), cmpMovies);
  if(movie_addr_offset == NULL){
    players.clear();
    return false;
  }
  //if it exists we have the offset and calculate the address
  int movie_offset = *(int*)movie_addr_offset;
  const char* movie_addr = (char*)movieFile + movie_offset;

  movie_offset = 0; //we use this to parse movie data
  movie_offset += movie.title.size() // go until null
               + 2; //go after year delta
  if(movie_offset % 2 != 0) ++movie_offset;
  short num_actors;
  memcpy(&num_actors, movie_addr + movie_offset, sizeof(short));
  movie_offset += 2;
  
  //get the offset to the start of the actor offset array
  if(movie_offset % 4 != 0) movie_offset += 2;

  populateActorList(actorFile, movie_addr + movie_offset,
                    num_actors, players);

  return true;
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
