#include <ctime>
#include <map>
#include <vector>
#include <list>
#include <set>
#include <string>
#include <iostream>
#include <iomanip>
#include "imdb.h"
#include "path.h"
using namespace std;

void setActors(const imdb& db, const string &start_actor,
               const string &end_actor, string &start, string &end){

  vector<film> cast;

  db.getCredits(start_actor, cast);
  int dim_start = cast.size();
  db.getCredits(end_actor, cast);
  int dim_end = cast.size();

  if(dim_start < dim_end) {
    start = end_actor;
    end = start_actor;
  }
  else{
    start = start_actor;
    end = end_actor;
  }
}

void generateShortestPath(const imdb& db, const string& start_actor,
                          const string& end_actor){
  list<path> partial_paths;
  set<string> actor_db;
  set<film> movie_db;                     
  string start, end;


  //1. first optimization, using the actor with the lowest number of movies
  //   as the starting actor
  setActors(db, start_actor, end_actor, start, end);

  //2. second optimization, using caching to store data already used
  
  map<string, vector<film> > actor_cache; 
  map<film, vector<string> > movie_cache;

  partial_paths.push_back(path(start));
  while(!partial_paths.empty() && partial_paths.front().getLength() <= 5){
    //get last actor of oldest path
    path current_path = partial_paths.front();
    partial_paths.pop_front();

    string current_actor = current_path.getLastPlayer();
    //get all movies the actor starred in
    vector<film> movies;
    map<string, vector<film> >::iterator actor_cache_index;
    actor_cache_index = actor_cache.find(current_actor);

    if(actor_cache_index != actor_cache.end()){
      movies = actor_cache_index->second;
    }
    else{
      db.getCredits(current_actor, movies);
      actor_cache[current_actor] = movies;
    }

    //for movie in all movies
    for(vector<film>::iterator movie = movies.begin();
        movie != movies.end();
        ++movie){

        //we insert the movie in the movie_db
        //if it's already present skip to the next movie
        std::pair<set<film>::iterator, bool> ret;
        ret = movie_db.insert(*movie);
        if(ret.second == false) continue;

        vector<string> costars;
        map<film, vector<string> >::iterator movie_cache_index;
        movie_cache_index = movie_cache.find(*movie);

        if(movie_cache_index != movie_cache.end()){
          costars = movie_cache_index->second;
        }
        else{
          db.getCast(*movie, costars);
          movie_cache[*movie] = costars;
        }

        for(vector<string>::iterator costar = costars.begin();
            costar != costars.end();
            ++costar){

          //we insert the costar in the actor_db
          //if it's already present skip to the next costar
          std::pair<set<string>::iterator, bool> actor_res;
          actor_res = actor_db.insert(*costar);
          if(actor_res.second == false) continue;

          //if both movie and actor not in db
          //copy current path, add link(movie, player)
          //and push to the end of stack

          path new_path = current_path;
          new_path.addConnection(*movie, *costar);
          if(*costar == end){
            if(end_actor != end){
              new_path.reverse();
            }
            std::cout <<std::endl << new_path << std::endl;
            return;
          }
          partial_paths.push_back(new_path);
        }
    }
  }
  cout << endl << "No path between those two people could be found."
       << endl << endl;
}

/**
 * Using the specified prompt, requests that the user supply
 * the name of an actor or actress.  The code returns
 * once the user has supplied a name for which some record within
 * the referenced imdb existsif (or if the user just hits return,
 * which is a signal that the empty string should just be returned.)
 *
 * @param prompt the text that should be used for the meaningful
 *               part of the user prompt.
 * @param db a reference to the imdb which can be used to confirm
 *           that a user's response is a legitimate one.
 * @return the name of the user-supplied actor or actress, or the
 *         empty string.
 */

static string promptForActor(const string& prompt, const imdb& db)
{
  string response;
  while (true) {
    cout << prompt << " [or <enter> to quit]: ";
    getline(cin, response);
    if (response == "") return "";
    vector<film> credits;
    if (db.getCredits(response, credits)) return response;
    cout << "We couldn't find \"" << response << "\" in the movie database. "
	 << "Please try again." << endl;
  }
}

/**
 * Serves as the main entry point for the six-degrees executable.
 * There are no parameters to speak of.
 *
 * @param argc the number of tokens passed to the command line to
 *             invoke this executable.  It's completely ignored
 *             here, because we don't expect any arguments.
 * @param argv the C strings making up the full command line.
 *             We expect argv[0] to be logically equivalent to
 *             "six-degrees" (or whatever absolute path was used to
 *             invoke the program), but otherwise these are ignored
 *             as well.
 * @return 0 if the program ends normally, and undefined otherwise.
 */

int main(int argc, const char *argv[])
{
  imdb db(determinePathToData(argv[1])); // inlined in imdb-utils.h
  if (!db.good()) {
    cout << "Failed to properly initialize the imdb database." << endl;
    cout << "Please check to make sure the source files exist and that you have permission to read them." << endl;
    exit(1);
  }
  
  while (true) {
    string source = promptForActor("Actor or actress", db);
    if (source == "") break;
    string target = promptForActor("Another actor or actress", db);
    if (target == "") break;
    if (source == target) {
      cout << "Good one.  This is only interesting if you specify two different people." << endl;
    } else {
      // replace the following line by a call to your
      //generateShortestPath routine... 
      time_t start = time(NULL);
      generateShortestPath(db, source, target);
      time_t end = time(NULL);
      std::cout << std::endl << difftime(end, start) << std::endl;
    }
  }
  
  cout << "Thanks for playing!" << endl;
  return 0;
}

