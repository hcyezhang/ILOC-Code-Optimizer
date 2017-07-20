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


unordered_map<string, cfg_node> graph; // global var used to store cfg
//unordered_map<string, string> const_table; //global const table that keeps track of constant variables<key = variable name, value = const>
//unordered_map<string, int> vn_table; //value number table <key=variable name, value = value number>
//unordered_map<int, unordered_set<string>> vn_inverse; //enables convinient inverse look-up of value number table;
//unordered_map<string, int> lvn_table;
unordered_set<string> commute_set({"add","mult"});
unordered_set<string> skip_set({"br", "cmp_LE", "cmp_LT", "cmp_GT", "cmp_GE", "cmp_NE", "cbr", "halt", "read", "write", "cmp_EQ", "store"});

int global_counter; //generate VN;

vector<string> work_list({"root"});
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
            //cout << w << " ";
        } while(w != name);
        //cout << endl;
        components.push_back(comps);
    }
}

int find_const(string key,vector<unordered_map<string, string>> predecessor_const_table){
    //if(local_const_table.find(key) != local_const_table.end()){ return local_const_table[key];}
    for(int i = predecessor_const_table.size()-1; i >= 0; i--){
        if (predecessor_const_table[i].find(key) != predecessor_const_table[i].end()) return i; 
    }
    return -1;
}

int find_vn(string key, vector<unordered_map<string, int>> predecessor_vn_table){
    for(int i = predecessor_vn_table.size()-1; i >= 0; i-- ){
        if (predecessor_vn_table[i].find(key) != predecessor_vn_table[i].end()) return i;
    }
    return -1;
}

int find_lvn(string key, vector<unordered_map<string, int>> &predecessor_lvn_table){
    for(int i = predecessor_lvn_table.size()-1; i >= 0; i--){
        if (predecessor_lvn_table[i].find(key) != predecessor_lvn_table[i].end()) return i;
    }
    return -1;
}

void assign_update(iloc_row &iloc, vector<unordered_map<string, int>> &predecessor_vn_table, vector<unordered_map<int, unordered_set<string>>> &predecessor_vn_inverse,  vector<unordered_map<string, string>> &predecessor_const_table, int counter){
    if(iloc.op == "loadI") predecessor_const_table.back()[iloc.target] = iloc.lvar; 
    //if it was a constant
    else if(predecessor_const_table.back().find(iloc.target) != predecessor_const_table.back().end())
        predecessor_const_table.back().erase(iloc.target);
    int vn_idx = find_vn(iloc.target, predecessor_vn_table); 
    // if the variable has been assigned previously in the current scope, we delete it from previous vn_inverse .
    if(vn_idx == predecessor_vn_table.size() - 1) predecessor_vn_inverse[vn_idx][predecessor_vn_table[vn_idx][iloc.target]].erase(iloc.target); 
    predecessor_vn_table.back()[iloc.target] = counter;
    predecessor_vn_inverse.back()[counter].insert(iloc.target);
}

void lvn(const string &node_name, vector<unordered_map<string, int>> &predecessor_vn_table, vector<unordered_map<int, unordered_set<string>>> &predecessor_vn_inverse,  vector<unordered_map<string, string>> &predecessor_const_table){
    unordered_map<string, string> local_const_table;
    predecessor_const_table.push_back(local_const_table);

    unordered_map<string, int> local_vn_table;
    predecessor_vn_table.push_back(local_vn_table);

    unordered_map<int, unordered_set<string>> local_vn_inverse;
    predecessor_vn_inverse.push_back(local_vn_inverse);

    int current_scope = predecessor_vn_table.size()-1;

    for(auto& iloc: graph[node_name].codes){
        if(skip_set.find(iloc.op) != skip_set.end()) continue;
        int v_lvar, v_rvar;
        int tmp1 = find_vn(iloc.lvar, predecessor_vn_table);
        //cout<< iloc.op << ", " << iloc.lvar << "," << iloc.rvar << ", "<< iloc.target << endl;
        if (iloc.op == "loadI" || iloc.op == "load" || iloc.op == "i2i"){
            //create vn for loadI and i2i, besides, lvar and target share the same 
            //vn
            if(iloc.op != "load"){
                //if it is a constant, use loadI
                if(tmp1 != -1 && ((predecessor_const_table[tmp1].find(iloc.lvar) != 
                                predecessor_const_table[tmp1].end()) || iloc.op == "loadI")) {
                    iloc.op = "loadI";  
                    if(predecessor_const_table[tmp1].find(iloc.lvar) != predecessor_const_table[tmp1].end()){
                        iloc.lvar = predecessor_const_table[tmp1][iloc.lvar];
                        assign_update(iloc, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table, predecessor_vn_table[tmp1][iloc.lvar]);
                    }
                    else
                        assign_update(iloc, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table, ++global_counter);
                }
                else if(tmp1 == -1){
                    //cout << iloc.lvar << iloc.target << endl;
                    predecessor_vn_table.back()[iloc.lvar] = ++global_counter;
                    predecessor_vn_inverse.back()[global_counter].insert(iloc.lvar);
                    assign_update(iloc, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table, global_counter);
                }
                else{
                    int vn = predecessor_vn_table[tmp1][iloc.lvar];
                    assign_update(iloc, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table, vn);
                }
            }
            //load from memory, the value is always unknown, so give it a new vn
            else assign_update(iloc, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table, ++global_counter);
            continue;
        }
        if(iloc.rvar == "") continue;
        //get the value number of lvar or rvar
        //if we cannot find a value number for lvar or rvar, add it to vn table
        if(tmp1 == -1){
            predecessor_vn_table.back()[iloc.lvar] = ++global_counter;
            tmp1 = predecessor_vn_table.size() - 1;
            predecessor_vn_inverse.back()[global_counter].insert(iloc.lvar);
        }
        int tmp2 = find_vn(iloc.rvar, predecessor_vn_table);
        if(tmp2 == -1){
            predecessor_vn_table.back()[iloc.rvar] = ++global_counter;
            tmp2 = predecessor_vn_table.size() - 1;
            predecessor_vn_inverse.back()[global_counter].insert(iloc.rvar);
        }
        //assign_update(iloc, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table, ++global_counter);
        //continue;
        // if lvar and rval are both const in the scope where they are defined, 
        // then we modify the iloc code with op = 'loadI'
        // if lvar is a constant, replace it
        int num_of_consts = 0;
        if(predecessor_const_table[tmp1].find(iloc.lvar) != predecessor_const_table[tmp1].end()
                && (predecessor_const_table[tmp2].find(iloc.rvar) != predecessor_const_table[tmp2].end()
                    || iloc.op[iloc.op.size()-1] == 'I')){
            cout << iloc.lvar << predecessor_const_table[tmp1][iloc.lvar] << endl;
            num_of_consts = 2;
        }
        else if(predecessor_const_table[tmp1].find(iloc.lvar) != predecessor_const_table[tmp1].end()){
            num_of_consts = 3;
        }
        else if(predecessor_const_table[tmp2].find(iloc.rvar) != predecessor_const_table[tmp2].end()){
            num_of_consts = 4;
        }

        if(num_of_consts == 2){
            //if(predecessor_const_table[tmp1].find(iloc.lvar) != predecessor_const_table[tmp1].end()
            //    && (predecessor_const_table[tmp2].find(iloc.rvar) != predecessor_const_table[tmp2].end()
            //      || iloc.op[iloc.op.size()-1] == 'I')){
            string old_op = iloc.op;
            iloc.op = "loadI";
            int lhs, rhs;
            //cout<< predecessor_const_table[tmp1][iloc.lvar] << endl;
            lhs = stoi(predecessor_const_table[tmp1][iloc.lvar]);
            if(old_op[old_op.size()-1] == 'I') {
                rhs = stoi(iloc.rvar);
                old_op = old_op.substr(0, old_op.size()-1);
            }
            else{
                //cout<< predecessor_const_table[tmp2][iloc.rvar] << endl;
                rhs = stoi(predecessor_const_table[tmp2][iloc.rvar]);
            }
            if(old_op == "add")
                iloc.lvar = to_string(lhs + rhs);
            else if(old_op == "mult")
                iloc.lvar = to_string(lhs * rhs);
            else if(old_op == "sub")
                iloc.lvar = to_string(lhs - rhs);
            else if(old_op == "div")
                iloc.lvar = to_string(lhs / rhs);
            iloc.rvar = "";
            //find the vn for the constant
            assign_update(iloc, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table, ++global_counter);
            continue;
        }
            else if(num_of_consts == 3 || num_of_consts == 4){
                string old_op = iloc.op;
                iloc.op = old_op + "I";
                if(num_of_consts == 3){
                    string tmp_var = iloc.lvar;
                    iloc.lvar = iloc.rvar;
                    iloc.rvar = predecessor_const_table[tmp1][tmp_var];
                }
                else {
                    iloc.rvar = predecessor_const_table[tmp2][iloc.rvar];
                }
            }
            //algebraic identity
            if(iloc.op[iloc.op.size()-1] == 'I'){
                if(iloc.op == "addI" || iloc.op == "subI"){
                    if(iloc.rvar == "0"){
                        iloc.op = "i2i";
                        iloc.rvar = "";
                        int vn = predecessor_vn_table[tmp1][iloc.lvar];
                        assign_update(iloc, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table, vn);
                        continue;
                    }
                }
                else if(iloc.op == "multI" || iloc.op == "divI"){
                    if(iloc.rvar == "1"){
                        iloc.op = "i2i";
                        iloc.rvar = "";
                        int vn = predecessor_vn_table[tmp1][iloc.lvar];
                        assign_update(iloc, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table, vn);
                        continue;
                    }
                }
            }
            v_lvar = predecessor_vn_table[tmp1][iloc.lvar];
            v_rvar = predecessor_vn_table[tmp2][iloc.rvar]; 
            //commute operationc checking
            if(commute_set.find(iloc.op) != commute_set.end()){
                if(v_lvar > v_rvar){
                    int tmp = v_lvar;
                    v_lvar = v_rvar;
                    v_rvar = tmp;
                }
            }
            string hash_key = to_string(v_lvar)  + iloc.op + to_string(v_rvar);
            int hash_key_idx = find_vn(hash_key, predecessor_vn_table);
            cout << iloc.lvar << iloc.op << iloc.rvar << ", " << hash_key_idx <<endl; 
            if(hash_key_idx != -1){
                int vn = predecessor_vn_table[hash_key_idx][hash_key];
                //check if the variable has been redefined, we need first collect the vn mappings
                //then check if each definition is reachable, which means the closet definition has the
                //value number we want
                unordered_set<string> vn_set;
                for(auto& pred: predecessor_vn_inverse){
                    if(pred.find(vn) != pred.end()){
                        vn_set.insert(pred[vn].begin(), pred[vn].end());
                    }
                }
                bool assigned(false);
                for(auto& var: vn_set){
                    int var_idx = find_vn(var, predecessor_vn_table);
                    //current var can reach our scope
                    //cout << var << ", " << predecessor_vn_table[var_idx][var] << endl;
                    if(predecessor_vn_table[var_idx][var] == vn){
                        iloc.lvar = var;
                        iloc.op = "i2i";
                        iloc.rvar = "";
                        assigned = true;
                        assign_update(iloc, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table, 
                                vn);
                        break;
                    }
                }
                if(assigned) continue;
            }
            predecessor_vn_table.back()[hash_key] = ++global_counter;
            assign_update(iloc, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table, global_counter);
            //predecessor_lvn_table.back()[hash_key] = global_counter;
        }
    }

    //generate cfg
    void read_file(const string& fileName){
        ifstream infile;
        infile.open(fileName);
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
                    else{
                        row.lvar = *(++it);
                        cout << row.lvar << endl;
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
        for(int i = 0; i < inds.size(); i++){
            //cout << blocks[i] << " " << inds[i][0] << " " << inds[i][1] << endl;
        }
        for(auto& row: file){
            //cout << row.op << " " << row.lvar << " " << row.rvar << " " << row.target << endl;
        }
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
        //for(auto& node: graph){
        //  cout << node.first << " " ;
        //  if(node.second.succs.size()) cout << node.second.succs[0] << " ";
        //  if(node.second.succs.size() > 1){
        //    cout << node.second.succs[1];
        //  }
        //  cout << endl;
        //}
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

    void svn(const string &node_name, vector<unordered_map<string, int>> &predecessor_vn_table, vector<unordered_map<int, unordered_set<string>>> &predecessor_vn_inverse,  vector<unordered_map<string, string>> &predecessor_const_table, unordered_set<string> & visited){
        if (visited.find(node_name) == visited.end()){
            visited.insert(node_name);
            lvn(node_name, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table);
            for(auto s: graph[node_name].succs){
                if(graph[s].preds.size() == 1) {
                    svn(s, predecessor_vn_table, predecessor_vn_inverse, predecessor_const_table,visited);
                }
                else {
                    if(visited.find(s) == visited.end())
                        work_list.push_back(s);
                }
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
        //vector<unordered_map <string, int>> pred_vn_table;
        //vector<unordered_map<string,string>> pre_const_table;
        //vector<unordered_map<int, unordered_set<string>>> pre_reverse_vn;

        unordered_set<string> duplicate_set;
        while(!work_list.empty()){
            vector<unordered_map <string, int>> pred_vn_table;
            vector<unordered_map<string,string>> pre_const_table;
            vector<unordered_map<int, unordered_set<string>>> pre_reverse_vn;

            string block = work_list.back(); 
            work_list.pop_back();
            //pred_vn_table.clear();
            //pre_reverse_vn.clear();
            //pre_const_table.clear();
            svn(block, pred_vn_table, pre_reverse_vn, pre_const_table, duplicate_set);
            //work_list.clear();
            //cout << work_list.size() << endl;
        }
        write_file(argv[2]);
    }
