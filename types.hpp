#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

class Game;

class InventoryEntry {
  public:
    std::string name;
    bool visible;
};

class PersistentCounter {
  public:
    PersistentCounter(const char *filename);
    int postIncrement();

  private:
    std::fstream file;
    int counter = 0;
};

class AbstractSceneResult {
  public:
    virtual void run(Game &game) = 0;
};

class SceneResult : public AbstractSceneResult {
  public:
    static SceneResult from_string(std::string);
    virtual void run(Game &game) override;
    std::optional<std::string> transition;
    std::optional<std::string> inventory;
    std::optional<std::pair<std::string, std::string>> replace;
};

class Scene {
  public:
    Scene(std::ifstream &&file);
    std::pair<AbstractSceneResult *, std::vector<SceneResult> *>
    run(Game &game);
    std::string key;

  private:
    std::string description;
    std::map<std::string, std::string> conditional_descriptions;
    std::map<std::string, std::string> choices;
    std::map<std::string, SceneResult> actions;
    std::vector<SceneResult> posts;
    std::optional<SceneResult> defaultAction;

    std::string parse_post(std::ifstream &file);
    std::string parse_conditional_descriptions(
        std::string_view header, std::ifstream &file
    );
    std::string parse_choices(std::ifstream &file);
    std::string parse_actions(std::ifstream &file);

    void show_description(Game &game);
    void show_choices();
};

class Game {
  public:
    void registerScene(std::shared_ptr<Scene> scene);
    void run(std::string key);
    std::map<std::string, bool> inventory;

  private:
    std::shared_ptr<Scene> currentScene;
    std::map<std::string, std::shared_ptr<Scene>> scenes;

    friend class SceneResult;
};
