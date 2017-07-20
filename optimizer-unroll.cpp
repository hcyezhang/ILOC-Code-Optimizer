#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
using namespace::std;
struct iloc_row{
    string op;
    string target;
    string lvar;
    string rvar;
};

struct cfg_node{
    vector<iloc_row> codes;
    vector<string> succs;
    vector<string> preds;
};


unordered_map<string, cfg_node> graph;

void dominance_solver(unordered_map<string, unordered_set<string>> &dom, unordered_set<string> &components){
    for(auto& comp:components){
        dom[comp] = unordered_set<string>();
    }
    for(auto& domi:dom){
        for(auto& comp:components){
            domi.second.insert(comp);
        }
    }
    dom["root"].clear();
    dom["root"].insert("root");
    bool changed = true;
    while(changed){
        changed = false;
        for(auto& comp: components){
            unordered_map<string, int> tmp;
            int count = 0;
            for(auto& pred: graph[comp].preds){
                //cout<< comp << "," << pred << endl;
                count += 1;
                //intersection of dom(predj)
                for(auto& domj: dom[pred]){
                    //cout<< comp << "," << domj << endl;
                    if(tmp.find(domj) != tmp.end()) tmp[domj] += 1;
                    else tmp[domj] = 1;
                }
            }
            unordered_set<string> intersection;
            intersection.insert(comp);
            for(auto& tmp_: tmp){
                if(tmp_.second == count) intersection.insert(tmp_.first);
            }
            //cout<< comp;
            //for(auto& inter:intersection) cout << inter<< ",";
            //cout<< endl;
            if(intersection != dom[comp]) {
                dom[comp].clear();
                dom[comp].insert(intersection.begin(), intersection.end());
                changed = true;
            }
        }
    }
}

bool loop_header_dfs(const string& name, const string& header, unordered_set<string> &visited, unordered_set<string> &tmp, unordered_set<string> &domain){
    visited.insert(name);
    bool reachable(false);
    for(auto& succ: graph[name].succs){
        //cout<< name << "," << succ <<"," << header << endl;
        if(domain.find(succ) != domain.end()){
            //if(visited.find(succ) != visited.end())
            if(succ == header) {
                tmp.insert(name);
                reachable = true;
            }
            else if(tmp.find(succ) != tmp.end()) {
                tmp.insert(name);
                reachable = true;
            }
            else if(visited.find(succ) == visited.end()){
                if(loop_header_dfs(succ, header, visited, tmp, domain)) {
                    tmp.insert(name);
                    reachable = true;
                }
            }
        }
    }
    return reachable;
}

void detect_connected(unordered_map<string, int>& lowlinks, unordered_map<string, int>& colors, unordered_map<string, int>& on_stack, vector<string>& stack, cfg_node& node, const string& name, int& index, vector<unordered_set<string>> &components){
    lowlinks[name] = index;
    colors[name] = index;
    on_stack[name] = 1;
    index++;
    stack.push_back(name);

    for(auto& succ: node.succs){
        if(colors.find(succ) == colors.end()){
            detect_connected(lowlinks, colors, on_stack, stack, graph[succ], succ, index, components);
            lowlinks[name] = min(lowlinks[name], lowlinks[succ]);
        }
        else if(on_stack[succ]){
            lowlinks[name] = min(lowlinks[name], lowlinks[succ]);
        }
    }

    if(lowlinks[name] == colors[name]){
        string w;
        unordered_set<string> comps;
        do{
            w = stack[stack.size() - 1];
            stack.pop_back();
            comps.insert(w);
            on_stack[w] = 0;
            cout << w << " ";
        } while(w != name);
        cout << endl;
        components.push_back(comps);
    }
}

void unroll_loop(int unroll_times){
    unordered_map<string, int> colors;  
    unordered_map<string, int> lowlinks;
    unordered_map<string, int> on_stack;
    vector<string> stack;
    vector<unordered_set<string>> connected_components;
    int index(0);
    //find all strongly connected components in the cfg
    for(auto& node: graph) {
        if(colors.find(node.first) == colors.end()) {
            detect_connected(lowlinks, colors, on_stack, stack, node.second, node.first, index, connected_components);
        }
    }
    //get dominator for each component in the cfg
    unordered_set<string> nodes_name;
    unordered_map<string, unordered_set<string>> dom;
    for(auto& node:graph) nodes_name.insert(node.first);
    dominance_solver(dom, nodes_name);
    //loop header is the dominator
    //remove root from dom
    for(auto& domi:dom){
        domi.second.erase("root");
    }

    int upper_bound(0);
    for(auto& node: graph){
        if(node.first != "root"){
            upper_bound = max(upper_bound, stoi(node.first.substr(1,node.first.size()-1)));
        }
    }
    //check each loop, find the header and latches for each loop
    for(auto& comps: connected_components){
        //complex loops have more than 1 components
        //if(comps.size() > 1){
        //cout<<"dom"<<endl;
        //for(auto& domi: dom) {
        //  cout << domi.first << ": ";
        //  for(auto& domij: domi.second) cout << domij << ", ";
        //  cout << endl;
        //}
        unordered_map<string, unordered_set<string>> dom_tree;
        for(auto& comp:comps){
            //possible loop headers
            for(auto& domi:dom[comp]){
                dom_tree[domi].insert(comp);
            }
        }
        cout<<"dom_tree"<<endl;
        for(auto& node: dom_tree) {
            cout << node.first << ": ";
            for(auto& x: node.second){
                cout << x << ",";
            }
            cout << endl;
        }
        //check if the children of dominator is reachable from its comps
        unordered_map<string, unordered_set<string>> natural_loop;
        for(auto& domi: dom_tree){
            unordered_set<string> tmp;
            unordered_set<string> visited;
            loop_header_dfs(domi.first, domi.first, visited, tmp, domi.second);
            natural_loop[domi.first] = tmp;
        }
        cout<<"natural_loop"<<endl; 
        for(auto& node: natural_loop) {
            cout << node.first << ": ";
            for(auto& x: node.second){
                cout << x << ",";
            }
            cout << endl;
        }

        unordered_set<string> loop_to_unroll;
        //find the inner most loop
        for(auto& loopi: natural_loop){
            bool inner_most(true);
            for(auto& next: loopi.second){
                if(next != loopi.first && natural_loop[next].size() != 0){
                    inner_most = false;
                    break;
                }
            }
            if(inner_most && natural_loop[loopi.first].size()){
                loop_to_unroll.insert(loopi.first);
            }
        }
        cout<<"loop to unroll"<<endl;
        //unroll this loop by rearanging cfg
        //latch is the predecessor of header
        for(auto& header: loop_to_unroll){
            //replicate the loop
            string latch, last_latch;
            for(auto& pred: graph[header].preds){
                if(natural_loop[header].find(pred) != natural_loop[header].end()){
                    latch = pred;
                    break;
                }
            }
            last_latch = latch;
            cout << header <<", " << latch << endl;
            for(int i = 0; i < unroll_times; i++){
                //record the new mapping
                unordered_map<string, string> new_names;
                for(auto& next: natural_loop[header]){
                    string name = "L"+to_string(++upper_bound);
                    new_names[next] = name;
                    //add new node into cfg
                    if(next != latch){
                        cfg_node new_node = graph[next];
                        graph[name] = new_node;
                    }
                    else{
                        cfg_node new_node = graph[last_latch];
                        graph[name] = new_node;
                    }
                }
                //let the latch of the last iteration point to the header of this iteration
                for(auto& next: natural_loop[header]){
                    //a normal block in the loop body
                    if(next != latch){
                        for(auto& succ: graph[new_names[next]].succs){
                            string bak = succ;
                            succ = new_names[succ];
                            if(succ == ""){
                                cout << "warning: " <<  header << " " << bak << " " << next << endl;
                            }

                        }
                    }
                    else if(next != header){
                        for(auto& pred: graph[new_names[next]].preds){
                            pred = new_names[pred];
                        }
                    }
                    if(next == header){
                        graph[new_names[next]].preds.clear();
                        graph[new_names[next]].preds.push_back(last_latch);
                    }
                }
                for(auto& succ: graph[last_latch].succs){
                    if(succ == header){
                        succ = new_names[header];
                        break;
                    }
                }
                //update the predecessor of header for completeness
                for(auto& pred: graph[header].preds){
                    if(pred == last_latch){
                        pred = new_names[latch]; 
                        break;
                    }
                }
                //update the name of last latch of the unrolled loop
                last_latch = new_names[latch];
            }
        }
        //}
    }
    for(auto& node: graph){
        cout << node.first << ": ";
        for(auto& succ: node.second.succs) {
            if(succ == "") cout << "warning ";
            cout << succ << ",";
        }
        cout << endl;
    }
}


void read_file(const string& file_name){
    ifstream infile;
    infile.open(file_name);
    string line;
    vector<iloc_row> file;
    vector<vector<int>> inds;
    vector<string> blocks;

    int count = 0;
    cfg_node new_node;
    inds.push_back(vector<int>(1,0));
    string previous_name = "root";
    blocks.push_back("root");

    while(getline(infile, line)){
        istringstream iss(line);
        istream_iterator<string> it(iss);
        istream_iterator<string> end;
        iloc_row row;
        int loc = 0;
        for(; it != end; it++, loc++){
            if(*it == "//") break;
            if((*it)[it->size() - 1] == ':'){
                cfg_node new_node;
                graph[it->substr(0, it->size() - 1)] = new_node;
                inds[inds.size()-1].push_back(count-1);
                previous_name = it->substr(0, it->size() - 1);
                inds.push_back(vector<int>(1,count));
                //inds[previous_name].push_back(count);
                blocks.push_back(previous_name);
                break;
            }
            else if(loc == 0){
                row.op = *it;
            }
            else if(loc == 1){
                if((*it)[it->size() - 1] == ',') row.lvar = it->substr(0, it->size() - 1);
                else if(*it != "->") row.lvar = *it;
                else {
                    row.lvar = *(++it);
                    break;
                }
            }
            else if(loc == 2){
                if(*it != "=>" && *it != "->") {
                    if((*it)[it->size() - 1] != ','){
                        row.rvar = *it;
                        loc--;
                    }
                    else row.rvar = it->substr(0, it->size() - 1);
                }
                else if(*it == "->"){
                    loc--;
                }
            }
            else if(loc == 3){
                row.target = *it;
                break;
            }
        }
        file.push_back(row);
        count++;
    }
    infile.close();
    inds[inds.size()-1].push_back(count-1);
    //for(int i = 0; i < inds.size(); i++){
    //cout << blocks[i] << " " << inds[i][0] << " " << inds[i][1] << endl;
    //}
    //for(auto& row: file){
    //cout << row.op << " " << row.lvar << " " << row.rvar << " " << row.target << endl;
    //}
    count = 0;
    for(int i = 0; i < inds.size(); i++){
        for(int j = inds[i][0]; j < inds[i][1]+1; j++) {
            graph[blocks[i]].codes.push_back(iloc_row(file[j]));
        }
        if(file[inds[i][1]].op == "cbr"){
            graph[blocks[i]].succs.push_back(file[inds[i][1]].rvar);
            graph[blocks[i]].succs.push_back(file[inds[i][1]].target);
            graph[file[inds[i][1]].rvar].preds.push_back(blocks[i]);
            graph[file[inds[i][1]].target].preds.push_back(blocks[i]);
        }
        else if(file[inds[i][1]].op == "br"){
            graph[blocks[i]].succs.push_back(file[inds[i][1]].lvar);
            graph[file[inds[i][1]].lvar].preds.push_back(blocks[i]);
        }
        else if( i < inds.size() - 1 ){
            graph[blocks[i]].succs.push_back(blocks[i+1]);
            graph[blocks[i+1]].preds.push_back(blocks[i]);
        }
    }
    cout <<"cfg" <<endl;
    for(auto& node: graph){
        cout << node.first << ": " ;
        if(node.second.succs.size()) cout << node.second.succs[0];
        if(node.second.succs.size() > 1){
            cout << ", " << node.second.succs[1];
        }
        cout << endl;
    }
}

void format_code(){
    int upper_bound = 0;
    unordered_set<string> variables;
    for(auto& node: graph){
        for(auto& row: node.second.codes){
            if(row.op == ""){
                continue;
            }
            if(row.op != "cbr" && row.op != "br" && row.op != "write" && row.op != "read" && row.op != "halt"){
                //three variables
                if(row.lvar[0] == 'r')
                    variables.insert(row.lvar);
                variables.insert(row.target);
                if(row.rvar != "" && row.op[row.op.size()-1] != 'I'){
                    variables.insert(row.rvar);
                }
            }
            else if(row.op == "read"){
                variables.insert(row.rvar);
            }

        }
    }
    for(auto& var: variables){
        upper_bound = max(upper_bound, stoi(var.substr(1,var.size()-1)));
    }
    cout << upper_bound << endl;
    for(auto& node: graph){
        unordered_map<string, string> new_names;
        for(auto& row: node.second.codes){
            if(new_names.find(row.lvar) != new_names.end()){
                row.lvar = new_names[row.lvar];
            }
            if(new_names.find(row.rvar) != new_names.end()){
                row.rvar = new_names[row.rvar];
            }

            //if(row.op == "add" || row.op == "sub" || row.op == "div" || row.op == "mult" 
            //	|| row.op == "addI" || row.op == "subI" || row.op == "divI" || row.op == "multI"){
            if(row.op != "store" && row.target != "" && (row.lvar == row.target || row.rvar == row.target || new_names.find(row.target) != new_names.end())) {
                cout << row.lvar << ", " << row.target << endl;
                string new_name = "r"+to_string(++upper_bound);
                new_names[row.target] = new_name;
                row.target = new_name;
            }
            else if(row.op == "store" && new_names.find(row.target) != new_names.end()){
                row.target = new_names[row.target];
            }
            //}
        }
        //recover the renamed variable
        int insert_location = node.second.codes.size() - 1;
        if(node.second.codes[insert_location].op == "cbr" || node.second.codes[insert_location].op == "br"){
            insert_location--;
        }
        for(auto& name: new_names){
            iloc_row new_line;
            new_line.op = "i2i";
            new_line.lvar = name.second;
            new_line.target = name.first;
            node.second.codes.insert(node.second.codes.begin()+insert_location, new_line);
        }
    }
}
void dfs_write(ofstream& outfile, const string& node, unordered_set<string>& visited){
    visited.insert(node);
    //print the code of this node
    if(node != "root") outfile << node << ": nop" << endl;
    for(auto& row: graph[node].codes){
        if(row.op == ""){
            continue;
        }
        if(row.op != "cbr" && row.op != "br" && row.op != "write" && row.op != "read" && row.op != "halt"){
            //three variables
            if(row.rvar == "")
                outfile << row.op << " " << row.lvar << " => " << row.target << endl;
            else
                outfile << row.op << " " << row.lvar << ", " << row.rvar << " => " << row.target << endl;
        }
        else if(row.op == "cbr"){
            outfile << row.op << " " << row.lvar << " -> " << graph[node].succs[0] << ", " << graph[node].succs[1] << endl;
        }
        else if(row.op == "br"){
            outfile << row.op << " -> " << graph[node].succs[0] << endl;
        }
        else if(row.op == "write"){
            outfile << row.op << " " << row.lvar << endl;
        }
        else if(row.op == "read"){
            outfile << row.op << " " << row.lvar << " " << row.rvar << endl;
        }
        else if(row.op == "halt"){
            outfile << row.op << endl;
        }
    }
    for(auto& succ: graph[node].succs){
        if(visited.find(succ) == visited.end()){
            dfs_write(outfile, succ, visited);
        }
    }
}

void write_file(const string& file_name){
    ofstream outfile;
    outfile.open(file_name);
    unordered_set<string> visited;
    dfs_write(outfile, "root", visited);
    outfile.close();
}

int main(int argc, char* argv[]){
    read_file(argv[1]);
    int unroll_times = atoi(argv[2]);
    int formate_code = atoi(argv[4]);
    if(formate_code)
        format_code();
    unroll_loop(unroll_times);
    write_file(argv[3]);
}
