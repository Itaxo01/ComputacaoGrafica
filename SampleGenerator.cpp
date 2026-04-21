#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <random>
#include <tuple>

using namespace std;

class SampleValidator {
public:
    static void validate(const string& file_path) {
        string path = file_path;
        if (path.length() < 4 || path.substr(path.length() - 4) != ".obj") {
            path += ".obj";
        }
        
        ifstream file(path);
        if (!file.is_open()) {
            cout << "[error] Failed to open file: " << path << endl;
            return;
        }
        
        string line;
        unsigned int count = 0;
        vector<tuple<float, float, float>> file_vertices;
        string current_name;
        
        cout << "Validating file: " << path << endl;

        int colorCount = 0;
        bool pending_bezier = false;
        while(getline(file, line)){
            if (!line.empty() && line[0] == '#') {
                istringstream css(line.substr(1));
                string tag; css >> tag;
                if (tag == "color") colorCount++;
                else if (tag == "type") { string t; css >> t; pending_bezier = (t == "bezier_curve"); }
                continue;
            }
            if (line.empty()) continue;

            istringstream iss(line);
            string type;
            iss >> type;

            if (type == "v") {
                float x, y, z, w = 1.0f;
                iss >> x >> y >> z;
                (void)w;
                file_vertices.emplace_back(x, y, z);
            } else if (type == "o" || type == "g") {
                iss >> current_name;
                pending_bezier = false;
            } else if (type == "p") {
                string v_str;
                bool valid = false;
                while (iss >> v_str) {
                    try {
                        int v_idx = stoi(v_str);
                        if (v_idx < 0) v_idx = (int)file_vertices.size() + v_idx + 1;
                        if (v_idx > 0 && v_idx <= (int)file_vertices.size()) valid = true;
                    } catch (...) {}
                }
                if (valid) count++;
            } else if (type == "l" || type == "f") {
                int resolved = 0;
                string v_str;
                while (iss >> v_str) {
                    try {
                        int v_idx = stoi(v_str);
                        if (v_idx < 0) v_idx = (int)file_vertices.size() + v_idx + 1;
                        if (v_idx > 0 && v_idx <= (int)file_vertices.size()) resolved++;
                    } catch (...) {}
                }
                if (type == "l" && pending_bezier) {
                    if (resolved >= 4 && (resolved - 1) % 3 == 0) count++;
                } else if (resolved >= (type == "l" ? 2 : 3)) {
                    count++;
                }
            }
        }
        cout << "Validation successful. Found " << file_vertices.size() << " vertices, "
             << count << " objects, " << colorCount << " colored." << endl;
    }
};

static mt19937 rng(random_device{}());

float getRandomFloat(float min_val, float max_val) {
    uniform_real_distribution<float> dis(min_val, max_val);
    return dis(rng);
}

int getRandomInt(int min_val, int max_val) {
    uniform_int_distribution<int> dis(min_val, max_val);
    return dis(rng);
}

// Writes "# color R G B 255\n" with random RGB to the file
void writeRandomColor(ofstream &file) {
    file << "# color " << getRandomInt(0, 255)
         << " "        << getRandomInt(0, 255)
         << " "        << getRandomInt(0, 255)
         << " 255\n";
}

void interactiveGenerator() {
    string filename;
    cout << "Enter output filename (will append .obj if missing): ";
    cin >> filename;

    if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".obj") {
        filename += ".obj";
    }

    float min_val = -100.0f;
    float max_val = 100.0f;
    char customRange;
    cout << "Use custom random range? (y/N): ";
    cin >> customRange;
    if (tolower(customRange) == 'y') {
        cout << "Enter min float value: ";
        cin >> min_val;
        cout << "Enter max float value: ";
        cin >> max_val;
    }

    int numPoints = 0, numLines = 0, numWireframes = 0, numPolygons = 0, numBeziers = 0;
    cout << "Enter number of point objects to generate: ";
    cin >> numPoints;
    cout << "Enter number of line objects to generate (each with 2 vertices): ";
    cin >> numLines;
    cout << "Enter number of wireframe objects to generate (each with 2-6 vertices): ";
    cin >> numWireframes;
    cout << "Enter number of polygon objects to generate (each with 3-6 vertices): ";
    cin >> numPolygons;
    cout << "Enter number of bezier curve objects to generate: ";
    cin >> numBeziers;
    int numBezierSegments = 1;
    if (numBeziers > 0) {
        cout << "Enter number of cubic segments per bezier curve: ";
        cin >> numBezierSegments;
        if (numBezierSegments < 1) numBezierSegments = 1;
    }

    ofstream file(filename);
    if (!file.is_open()) {
        cout << "[error] Failed to open file for writing: " << filename << endl;
        return;
    }

    file << "# Generated by SampleGenerator\n";

    int vertex_count = 1;

    // Generate Points
    for (int i = 0; i < numPoints; ++i) {
        file << "o Point_" << i << "\n";
        writeRandomColor(file);
        file << "v " << getRandomFloat(min_val, max_val)
             << " "  << getRandomFloat(min_val, max_val)
             << " "  << getRandomFloat(min_val, max_val) << "\n";
        file << "p " << vertex_count << "\n";
        vertex_count++;
    }

    // Generate Lines
    for (int i = 0; i < numLines; ++i) {
        file << "o Line_" << i << "\n";
        writeRandomColor(file);
        file << "v " << getRandomFloat(min_val, max_val)
             << " "  << getRandomFloat(min_val, max_val)
             << " "  << getRandomFloat(min_val, max_val) << "\n";
        file << "v " << getRandomFloat(min_val, max_val)
             << " "  << getRandomFloat(min_val, max_val)
             << " "  << getRandomFloat(min_val, max_val) << "\n";
        file << "l " << vertex_count << " " << vertex_count + 1 << "\n";
        vertex_count += 2;
    }

    // Generate Wireframes (open polylines via 'l')
    for (int i = 0; i < numWireframes; ++i) {
        int polySize = getRandomInt(2, 6);
        file << "o Wireframe_" << i << "\n";
        writeRandomColor(file);
        for (int j = 0; j < polySize; j++) {
            file << "v " << getRandomFloat(min_val, max_val)
                 << " "  << getRandomFloat(min_val, max_val)
                 << " "  << getRandomFloat(min_val, max_val) << "\n";
        }
        file << "l";
        for (int j = 0; j < polySize; j++)
            file << " " << vertex_count + j;
        file << "\n";
        vertex_count += polySize;
    }

    // Generate Polygons (closed faces via 'f', randomly filled or outline)
    for (int i = 0; i < numPolygons; ++i) {
        int polySize = getRandomInt(3, 6);
        bool filled  = getRandomInt(0, 1) == 1;
        file << "o Polygon_" << i << "\n";
        writeRandomColor(file);
        file << "# filled " << (filled ? 1 : 0) << "\n";
        for (int j = 0; j < polySize; j++) {
            file << "v " << getRandomFloat(min_val, max_val)
                 << " "  << getRandomFloat(min_val, max_val)
                 << " "  << getRandomFloat(min_val, max_val) << "\n";
        }
        file << "f";
        for (int j = 0; j < polySize; j++)
            file << " " << vertex_count + j;
        file << "\n";
        vertex_count += polySize;
    }

    // Generate Bezier Curves
    // Format: {P0, C0, C1, P1, C2, C3, P2, ...} — anchors at indices 0,3,6,...
    for (int i = 0; i < numBeziers; ++i) {
        int numPts = 1 + numBezierSegments * 3; // 4 for 1 seg, 7 for 2, 10 for 3, ...
        file << "o BezierCurve_" << i << "\n";
        writeRandomColor(file);
        file << "# type bezier_curve\n";
        for (int j = 0; j < numPts; j++) {
            file << "v " << getRandomFloat(min_val, max_val)
                 << " "  << getRandomFloat(min_val, max_val)
                 << " 0.0\n";
        }
        file << "l";
        for (int j = 0; j < numPts; j++)
            file << " " << vertex_count + j;
        file << "\n";
        vertex_count += numPts;
    }

    file.close();
    cout << "Successfully wrote to " << filename << endl;

    // Auto validate
    cout << "\nRunning SampleValidator..." << endl;
    SampleValidator::validate(filename);
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        string cmd = argv[1];
        if (cmd == "validate" && argc > 2) {
            SampleValidator::validate(argv[2]);
            return 0;
        } else {
            cout << "Usage:\n";
            cout << "  " << argv[0] << "                   (Interactive generation)\n";
            cout << "  " << argv[0] << " validate <file.obj> (Validate existing obj file)\n";
            return 1;
        }
    }

    interactiveGenerator();
    return 0;
}