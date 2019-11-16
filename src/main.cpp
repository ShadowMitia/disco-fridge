#include <fmt/format.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <fstream>
#include <algorithm>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

struct SDLMixerMusicDestructor {
  void operator()(Mix_Music* music) { Mix_FreeMusic(music); }
};

using MixMusicPtr = std::unique_ptr<Mix_Music, SDLMixerMusicDestructor>;

std::string toLowercase(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c){ return std::tolower(c); });
  return str;
}

std::vector<std::string> split(std::string const& s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

struct SongManager {
private:
  std::mt19937 gen{std::random_device()()};
  std::vector<std::filesystem::path> songs;

public:

  std::size_t size() const {
    return songs.size();
  }

  void addSong(std::filesystem::path songPath) {
    songs.push_back(songPath);
  }

  void removeSong(std::filesystem::path songPath) {
    for (std::size_t i = 0; i < songs.size(); i++) {
      if (songs[i] == songPath) {
        songs.erase(std::begin(songs) + static_cast<long int>(i));
        return;
      }
    }
  }

  std::string getRandomSong() {
    std::string song;
    if (songs.size() > 0) {
      std::uniform_int_distribution<std::size_t> dis{0, songs.size() - 1};
      song = songs[dis(gen)].string();
    }
    return song;
  }
};


void waitForMilliseconds(std::chrono::milliseconds milliseconds) {
  std::this_thread::sleep_for(milliseconds);
}



void updateSongs(SongManager& songManager, std::filesystem::path pathToSongs, std::filesystem::path archivePath, std::vector<std::string> const& playlists) {

  std::filesystem::remove(archivePath);

  for (std::string url : playlists) {
    fmt::print("Downloading {}\n", url);
    std::string youtubePlaylistDL =
      fmt::format("youtube-dl --ignore-errors"
                  " --no-warnings"
                  " --prefer-free-formats "
                  " --ignore-errors"
                  " --no-overwrites"
                  " -o '{}/%(id)s.%(ext)s'"
                  " --extract-audio "
                  " --download-archive '{}'"
                  " --quiet '{}' --no-call-home ",
                  pathToSongs.string(), archivePath.string(), url);
    std::system(youtubePlaylistDL.c_str());
  }

  std::fstream archiveFile(archivePath, std::ios::in);
  std::string lineArchive;

  std::vector<std::string> songIds;

  while (std::getline(archiveFile, lineArchive)) {
    auto data = split(lineArchive, ' ');
    auto songName = data[1];
    songIds.push_back(songName);
  }

  for(auto& p: std::filesystem::directory_iterator(pathToSongs)) {
    auto it = std::find(std::begin(songIds), std::end(songIds), p.path().stem());
    if (it != std::end(songIds)) {
      songManager.addSong(p.path());
    } else {
      std::filesystem::remove(p.path());
      songManager.removeSong(p.path());
    }
  }

}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  fmt::print("Disco Fridge !\n");

  std::filesystem::path pathToDiscoFridgeRootFolder("/home/dimitri/disco-fridge");
  std::filesystem::path pathToSongs(pathToDiscoFridgeRootFolder / "songs");
  std::filesystem::path archivePath(pathToDiscoFridgeRootFolder / "archive.txt");
  std::chrono::seconds updateDelay{60};
  std::vector<std::string> playlists{
                                     "https://www.youtube.com/playlist?list=PLeo8Y1fcAuPeODOtgrtYpzaLDuejCLcIY",
                                     "https://www.youtube.com/watch?v=NvS351QKFV4&list=PL9295WRjvNiwjb_ZStMtiy90Pyu_WayDE"};

  std::filesystem::create_directories(pathToSongs);
 
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    fmt::print("Couldn't load SDL");
    return -1;
  }

  if( Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096 ) == -1 )
    {
      fmt::print("Couldn't load SDL_mixer");
      return -1;
    }

  SongManager songManager;

  for(auto& p: std::filesystem::directory_iterator(pathToSongs)) {
    songManager.addSong(p.path());
  }

  std::thread thread([&](){
                       while (true) {
                         updateSongs(songManager, pathToSongs, archivePath, playlists);
                         waitForMilliseconds(updateDelay);
                       }
                     });
    thread.detach();

  MixMusicPtr music = nullptr;

  bool isLooping = true;

  while (isLooping) {
    if (songManager.size() > 0) {
      if (Mix_PlayingMusic() == 0) {
        std::string song = songManager.getRandomSong();
        fmt::print("Currently playing : {}\n", song);
        music.reset(Mix_LoadMUS(song.c_str()));

        if (music == nullptr) {
          fmt::print("Couldn't load music {}\n", song);
          continue;
        }
        if (Mix_PlayMusic(music.get(), 1) == -1) {
          fmt::print("Couldn't play music : {}\n", song) ;
        }
      }
    } else {
      fmt::print("No music found...\n");
    }

    std::chrono::seconds timespan(5);
    waitForMilliseconds(timespan);
  }

  fmt::print("Exiting... Bring disco back!\n");

  Mix_CloseAudio();
  Mix_Quit();
}
