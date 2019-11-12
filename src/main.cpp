#include <fmt/format.h>
#include <fmt/ostream.h>

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

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;


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

void downloadPlaylist(std::string url, std::filesystem::path targetFolder, std::filesystem::path archiveFile) {
  fmt::print("Downloading playlist\n");
  std::string youtubePlaylistDL =
    fmt::format("youtube-dl --ignore-errors "
                "--prefer-free-formats "
                " -o '{}/%(title)s.%(ext)s' --extract-audio "
                " --download-archive '{}'"
                " --quiet '{}' --no-call-home ",
                targetFolder.string(), archiveFile.string(), url);

  std::system(youtubePlaylistDL.c_str());
}

void getNamesInPlaylist(std::string url, std::string output) {
  std::string youtubePlaylistNames =
    fmt::format("youtube-dl --restrict-filenames --flat-playlist -J --no-call-home --quiet --ignore-errors '{}' > {}", url, output);
  std::system(youtubePlaylistNames.c_str());
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
    fmt::print("Adding entry : {}\n", songPath.c_str());
    songs.push_back(songPath);
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

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  fmt::print("Disco Fridge !\n");

  std::filesystem::path pathToDiscoFridgeRootFolder("/home/dimitri/disco-fridge");
  std::filesystem::path pathToSongs(pathToDiscoFridgeRootFolder / "songs");
  std::filesystem::path tmpNames(std::filesystem::temp_directory_path() / "disco_fridge_downloadNames");
  std::filesystem::path archivePath(pathToDiscoFridgeRootFolder / "archive.txt");

  std::string playlist = "https://www.youtube.com/playlist?list=PL9295WRjvNiwjb_ZStMtiy90Pyu_WayDE";


  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    fmt::print("Couldn't load SDL");
    return -1;
  }

  if( Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096 ) == -1 )
    {
      fmt::print("Couldn't load SDL_mixer");
      return -1;
    }


  std::vector<std::string> names;

  getNamesInPlaylist(playlist, tmpNames);

  std::fstream f(tmpNames, std::ios::in);

  json j;
  f >> j;

  for (auto entry : j["entries"]) {
    std::string title = entry["title"];
    fmt::print("title {}\n", title);
    names.push_back(entry["title"]);
  }

  std::fstream fileOfNames(tmpNames.string(), std::ios::in);

  downloadPlaylist(playlist, pathToSongs, archivePath);

  SongManager songManager;

  bool archiveChanged = false;
  for (std::filesystem::path path : std::filesystem::directory_iterator(pathToSongs)) {
    bool found = false;
    std::string pathName = path.stem();
    for (std::string name : names) {
      if (pathName.find(name) != std::string::npos) {
        fmt::print("Adding {}\n", name);
        songManager.addSong(path);
        found = true;
      }
    }

    if (!found) {
      fmt::print("Removing {}\n", pathName);
      archiveChanged = true;
      std::filesystem::remove(path);

      std::fstream file(archivePath, std::ios::out);
      for (auto entry : j["entries"]) {
        std::string output = fmt::format("{} {}\n", toLowercase(entry["ie_key"]), entry["id"]);
        file << output;
      }
    }
  }

  if (archiveChanged) {
    std::filesystem::remove(archivePath);
  }

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
          fmt::print("Couldn't play music\nExiting...\n");
          return 1;
        }
      }
    }

    std::chrono::seconds timespan(1);
    waitForMilliseconds(timespan);
  }

  fmt::print("Exiting... Bring disco back!\n");

  Mix_CloseAudio();
}
