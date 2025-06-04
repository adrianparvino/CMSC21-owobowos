#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "types.hpp"

PersistentCounter::PersistentCounter(const char *filename)
    : file(filename, std::ios::in | std::ios::out) {
    if (file.is_open()) {
        file.seekg(0);
        file >> counter;
    }
}

int PersistentCounter::operator++() {
    int x = ++counter;

    if (file.is_open()) {
        file.clear();
        file.seekp(0);
        file << counter << std::endl;
        file.flush();
    }

    return x;
}

int PersistentCounter::operator++(int) {
    int x = counter++;

    if (file.is_open()) {
        file.clear();
        file.seekp(0);
        file << counter << std::endl;
        file.flush();
    }

    return x;
}

int PersistentCounter::operator*() { return counter; }

void SceneResult::run(Game &game) const {
    if (transition.has_value()) {
        auto key = transition.value();
        for (auto key_ = game.replacements.find(key);
             key_ != game.replacements.end();
             key_ = game.replacements.find(key)) {
            key = key_->second;
        }
        auto nextScene = game.scenes.find(key);

        if (nextScene == game.scenes.end()) {
            std::cout << "Invalid scene key." << std::endl;
            return;
        }

        game.currentScene = nextScene->second;
    }

    if (replace.has_value()) {
        auto [from, to] = *replace;
        game.replacements[from] = to;
    }

    if (increment_counter) { game.counter++; }
}

Scene::Scene(std::ifstream &&file) {
    std::string line;

    while (std::getline(file, line) && line.empty()) {}
    key = line;

    while (std::getline(file, line) && line.empty()) {}
    description += line;
    while (std::getline(file, line)) {
        if (line.front() == '[' && line != "[COUNTER]") { break; }
        description += '\n';
        description += line;
    }

    while (file.peek() != EOF) {
        if (line == "[ACTIONS]") {
            line = parse_actions(file);
        } else if (line == "[CHOICES]") {
            line = parse_choices(file);
        } else if (auto sv = std::string_view(line); sv.substr(0, 2) == "[+") {
            auto end = sv.find(']');
            line = parse_conditional_descriptions(sv.substr(2, end - 2), file);
        } else if (line == "[POST]") {
            line = parse_post(file);
        } else if (line == "[AUTO]") {
            line = parse_auto(file);
        } else {
            getline(file, line);
        }
    }
}

std::pair<AbstractSceneResult *, std::vector<SceneResult> *>
Scene::run(Game &game) {
    show_description(game);
    show_choices();

    if (autoAction.has_value()) { return {&autoAction.value(), &posts}; }

    for (;;) {
        std::string input;
        std::cout << ">>> ";
        std::getline(std::cin, input);
        std::cout << std::endl;

        if (input == "quit") { exit(0); }
        if (input == "help") {
            show_choices();
            continue;
        }

        auto action = actions.find(input);
        if (action != actions.end()) { return {&action->second, &posts}; }
        if (defaultAction.has_value()) {
            return {&defaultAction.value(), &posts};
        }

        std::cout << "Invalid action." << std::endl;
    }
};

void Scene::show_description(Game &game) {
    static std::string counter_string("[COUNTER]");

    std::string description = this->description;

    for (auto &[key, value] : conditional_descriptions) {
        if (game.inventory.find(key) != game.inventory.end()) {
            description = value;
            break;
        }
    }

    auto found_counter = description.find(counter_string);
    if (found_counter != std::string::npos) {
        std::string_view description_view = description;

        do {
            std::cout << description_view.substr(0, found_counter)
                      << *game.counter;

            description_view.remove_prefix(
                found_counter + counter_string.size()
            );
        } while ((found_counter = description_view.find(counter_string)) !=
                 std::string::npos);

        std::cout << description_view << std::endl;
    } else {
        std::cout << description << std::endl;
    }
}

void Scene::show_choices() {
    for (auto const &choice : choices) {
        std::cout << "- " << choice << std::endl;
    }
}

std::string Scene::parse_choices(std::ifstream &file) {
    std::string line;

    while (std::getline(file, line)) {
        if (line.front() == '[') { break; }
        if (line.front() != '-') { continue; }

        auto start = line.find('[');

        choices.push_back(line.substr(start));
    }

    return line;
}

std::string Scene::parse_post(std::ifstream &file) {
    std::string line;

    while (std::getline(file, line)) {
        if (line.front() == '[') { break; }
        if (line.front() != '-') { continue; }

        SceneResult result;

        auto start = line.find("COUNTER++");
        if (start != std::string::npos) {
            result.increment_counter = true;
        } else if ((start = line.find('/')) != std::string::npos) {
            auto end = line.find('/', start + 1);

            std::string from = line.substr(start + 1, end - start - 1);
            std::string to = line.substr(end + 1);

            result.replace = std::make_pair(from, to);
        } else {
            continue;
        }

        posts.push_back(result);
    }

    return line;
}

std::string Scene::parse_conditional_descriptions(
    std::string_view header, std::ifstream &file
) {
    std::string line;

    auto &entry = conditional_descriptions[std::string(header)];
    std::getline(file, line);
    entry += line;
    while (std::getline(file, line)) {
        if (line.front() == '[') { break; }

        entry += '\n';
        entry += line;
    }

    return line;
}

std::string Scene::parse_actions(std::ifstream &file) {
    std::string line;

    while (std::getline(file, line)) {
        if (line.front() == '[') { break; }
        if (line.front() != '-') { continue; }

        auto start = line.find('[');
        auto end = line.find(']');

        SceneResult result;
        if (auto delim = line.find("/", start || 0);
            delim != std::string::npos) {
            result.transition = line.substr(delim + 1);
        }

        if (start != std::string::npos) {
            std::string command = line.substr(start + 1, end - start - 1);
            actions[command] = result;
        } else {
            defaultAction = result;
        }
    }

    return line;
}

std::string Scene::parse_auto(std::ifstream &file) {
    std::string line;

    while (std::getline(file, line)) {
        if (line.front() == '[') { break; }
        if (line.front() != '-') { continue; }

        SceneResult result;

        auto start = line.find('/');
        result.transition = line.substr(start + 1);

        autoAction = result;
    }

    return line;
}

Game::Game(PersistentCounter &counter) : counter(counter) {}

void Game::registerScene(std::shared_ptr<Scene> &&scene) {
    scenes.insert(std::make_pair(scene->key, scene));
}

void Game::run(std::string key) {
    auto scene = scenes.find(key);
    if (scene == scenes.end()) {
        std::cout << "Invalid scene key." << std::endl;
        return;
    }

    currentScene = scene->second;
    for (;;) {
        auto [sceneResult, posts] = currentScene->run(*this);

        if (sceneResult == nullptr) { continue; }
        auto key = currentScene->key;
        for (auto key_ = replacements.find(key); key_ != replacements.end();
             key_ = replacements.find(key)) {
            key = key_->second;
        }
        inventory[key] = false;
        for (auto const &post : *posts) { post.run(*this); }
        sceneResult->run(*this);
    }
}

int main() {
    static PersistentCounter counter("played.txt");

    Game game(counter);
    for (auto const &file : std::filesystem::directory_iterator("scenes")) {
        game.registerScene(
            std::shared_ptr<Scene>(new Scene(std::ifstream(file.path())))
        );
    }

    game.run("INTRODUCTION");

    return 0;
}