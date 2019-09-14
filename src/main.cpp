#include <fmt/format.h>
#include <wiringPi.h>
#include <chrono>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
#include <random>
#include <cstdlib>
#include <filesystem>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <curl/curl.h>


constexpr int targetPin = 7; // GPIO4
constexpr char const * const songURL = "http://example.com/";

std::vector<std::string> split(std::string const& s, char delimiter)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream , token, delimiter))
    {
      tokens.push_back(token);
    }
  return tokens;
}


static int writer(char *data, size_t size, size_t nmemb,
                  std::string *writerData)
{
  if(writerData == nullptr)
    return 0;

  writerData->append(data, size*nmemb);

  return size * nmemb;
}

int main(int argc, char* argv[]) {
  fmt::print("Disco Fridge !!!\n");


  char errorBuffer[CURL_ERROR_SIZE];
  std::string buffer;
  
  CURL* curl = curl_easy_init();
  CURLcode resultCode;

  if (!curl) {
    fmt::print("Couldn't initialise CURL\n");
    return -1;
  }

  resultCode = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
  if (resultCode != CURLE_OK) {
    fmt::print("Coudln't set error buffer {}\n", resultCode);
    return -1;
  }

  resultCode = curl_easy_setopt(curl, CURLOPT_URL, songURL);
  if (resultCode != CURLE_OK) {
    fmt::print("Couldn't set URL {}\n", errorBuffer);
    return -1;
  }

  resultCode = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  if (resultCode != CURLE_OK) {
    fmt::print("Couldn't set redirect option {}\n", errorBuffer);
    return -1;
  }


  resultCode = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
  if (resultCode != CURLE_OK) {
    fmt::print("Couldn't set write function {}\n", errorBuffer);
    return -1;
  }


  resultCode = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
  if (resultCode != CURLE_OK) {
    fmt::print("Couldn't set write buffer {}\n", errorBuffer);
    return -1;
  }

  resultCode = curl_easy_perform(curl);
  if (resultCode != CURLE_OK) {
    fmt::print("Couldn't get page {}\n", errorBuffer);
    return -1;
  }

   
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    fmt::print("Couldn't load SDL");
    return -1;
  }

  if( Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096 ) == -1 )
    {
          fmt::print("Couldn't load SDL_mixer");
        return -1;
    }

  // initialise wiringPi and setup for raspberry pi
  //  wiringPiSetup () ;

  //pinMode(targetPin, INPUT);
  //pullUpDnControl(targetPin, PUD_DOWN);

  std::vector<std::string> songsURL = {
                                    "https://www.youtube.com/watch?v=JWay7CDEyAI",
                                    "https://www.youtube.com/watch?v=I_izvAbhExY",
                                    "https://www.youtube.com/watch?v=16y1AkoZkmQ",
                                    "https://www.youtube.com/watch?v=rY0WxgSXdEE"
  };



  for (auto song : songsURL) {
    fmt::print("Line : {}\n", song);

    auto youtubeDLCommand = "youtube-dl -i --prefer-free-formats -o '~/disco-fridge/songs/%(title)s.%(ext)s'  --download-archive ~/disco-fridge/archive.txt  --extract-audio " + song;

    std::system(youtubeDLCommand.c_str());
  }


  std::vector<std::filesystem::path> songs;

  std::filesystem::path songsPath = "/home/dimitri/disco-fridge/songs";
  for (const auto & entry : std::filesystem::directory_iterator(songsPath)) {
    fmt::print("Entry : {}\n", entry.path().c_str());
    songs.emplace_back(entry.path());
  }

  std::random_device rd;  //Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<int> dis(0, songs.size() - 1);


  auto s = songs[dis(gen)].c_str();
  Mix_Music *music = Mix_LoadMUS( s );

  if (music == nullptr) {
    fmt::print("Couldn't load music {}\n", s);
    return 1;
  }

  /*
  bool isLooping = true;
  while (isLooping) {
    int value = digitalRead(targetPin);
    fmt::print("PIN {}\n", value);

    std::chrono::milliseconds timespan(1000);
    std::this_thread::sleep_for(timespan);
  }
  */

  if( Mix_PlayingMusic() == 0 )
    {
      if( Mix_PlayMusic( music, -1 ) == -1 )
        {
          fmt::print("Couldn't play music");
          return 1;
        }
    }


  std::chrono::milliseconds timespan(2000);
  std::this_thread::sleep_for(timespan);

  Mix_HaltMusic();

 s = songs[dis(gen)].c_str();
 music = Mix_LoadMUS( s );

  if (music == nullptr) {
    fmt::print("Couldn't load music {}\n", s);
    return 1;
  }


  if( Mix_PlayingMusic() == 0 )
    {
      if( Mix_PlayMusic( music, -1 ) == -1 )
        {
          fmt::print("Couldn't play music");
          return 1;
        }
    }


  std::this_thread::sleep_for(timespan);

  Mix_HaltMusic();

    s = songs[dis(gen)].c_str();
    music = Mix_LoadMUS( s );

  if (music == nullptr) {
    fmt::print("Couldn't load music {}\n", s);
    return 1;
  }

    if( Mix_PlayingMusic() == 0 )
    {
      if( Mix_PlayMusic( music, -1 ) == -1 )
        {
          fmt::print("Couldn't play music");
          return 1;
        }
    }



  std::this_thread::sleep_for(timespan);



  fmt::print("Exiting... Bring disco back!\n");
  Mix_FreeMusic( music );
  curl_easy_cleanup(curl);
  Mix_CloseAudio();
  SDL_Quit();
}
