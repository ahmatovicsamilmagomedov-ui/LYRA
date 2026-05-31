#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <stdexcept>

enum class Op { LOG, ADD, SUB, CMP, JMP, JMP_IF, HALT, DUMP };

struct Instruction {
    Op op;
    std::string arg1, arg2, target;
};

class FluxVM {
private:
    std::map<std::string, float> vram_num;
    std::vector<Instruction> bytecode;
    std::map<std::string, int> labels;
    std::set<std::string> loaded_files; // Защита от циклических импортов
    int ip = 0;
    bool cmp_flag = false;

    void compile(const std::string& filename) {
        // Если файл уже загружен, пропускаем, чтобы избежать рекурсии
        if (loaded_files.count(filename)) return;
        
        std::ifstream file(filename);
        if (!file.is_open()) throw std::runtime_error("Не удалось открыть файл: " + filename);
        
        loaded_files.insert(filename); // Отмечаем файл как загруженный

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line.substr(0, 2) == "//") continue;

            // РЕАЛИЗАЦИЯ ИМПОРТА с проверкой
            if (line.substr(0, 6) == "import") {
                size_t first = line.find('"');
                size_t last = line.find_last_of('"');
                if (first != std::string::npos && last != std::string::npos) {
                    std::string lib_path = line.substr(first + 1, last - first - 1);
                    compile(lib_path);
                }
                continue;
            }

            // Обработка меток
            if (line.back() == ':') {
                labels[line.substr(0, line.size() - 1)] = (int)bytecode.size();
                continue;
            }

            std::stringstream ss(line);
            std::vector<std::string> t;
            std::string temp;
            while (ss >> temp) t.push_back(temp);

            if (t.empty()) continue;

            if (t[0] == "log") bytecode.push_back({Op::LOG, t[1]});
            else if (t[0] == "cmp") bytecode.push_back({Op::CMP, t[1], t[2]});
            else if (t[0] == "jmp") bytecode.push_back({Op::JMP, t[1]});
            else if (t[0] == "jmp_if") bytecode.push_back({Op::JMP_IF, t[1]});
            else if (t[0] == "dump") bytecode.push_back({Op::DUMP});
            else if (t[0] == "~math") {
                Op math_op = (t[2] == "+") ? Op::ADD : Op::SUB;
                bytecode.push_back({math_op, t[1], t[3], t[5]});
            }
        }
    }

    float get_val(const std::string& arg) {
        try { return vram_num.count(arg) ? vram_num[arg] : std::stof(arg); }
        catch (...) { throw std::runtime_error("Неверный операнд: " + arg); }
    }

public:
    void run(const std::string& filename) {
        compile(filename);
        while (ip < (int)bytecode.size()) {
            Instruction& i = bytecode[ip];
            switch (i.op) {
                case Op::LOG: std::cout << "[LOG] " << i.arg1 << std::endl; break;
                case Op::DUMP: {
                    std::cout << "--- DUMP (IP: " << ip << ") ---" << std::endl;
                    for (auto const& [key, val] : vram_num) std::cout << key << ": " << val << std::endl;
                } break;
                case Op::CMP: cmp_flag = (get_val(i.arg1) == get_val(i.arg2)); break;
                case Op::ADD: vram_num[i.target] = get_val(i.arg1) + get_val(i.arg2); break;
                case Op::SUB: vram_num[i.target] = get_val(i.arg1) - get_val(i.arg2); break;
                case Op::JMP: 
                case Op::JMP_IF: {
                    if (i.op == Op::JMP_IF && !cmp_flag) break;
                    if (labels.find(i.arg1) == labels.end()) throw std::runtime_error("Метка не найдена: " + i.arg1);
                    ip = labels[i.arg1] - 1; 
                } break;
                case Op::HALT: return;
            }
            ip++;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "FLUX VM v3.1 | Запуск: ./Octa <main.Octa>" << std::endl;
        return 1;
    }
    try {
        FluxVM vm;
        vm.run(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "[CRITICAL ERROR] " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
