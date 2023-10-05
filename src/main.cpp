#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>


static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 200;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075;
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025;
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;
const double ENTITY_CREATE_PROBABILITY = 0.01;

// Type definitions
enum entity_type_t
{
    empty,
    plant,
    herbivore,
    carnivore
};

struct pos_t
{
    uint32_t i;
    uint32_t j;
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
};

int iteration = 1;

//prototipos
void initilize_entity (entity_t entity, int total_ent);
bool isNoEntitity(pos_t pos);
void simulate_ent(pos_t pos);
void simulate_plant(pos_t pos);
void simulate_herb(pos_t pos);
void simulate_carn(pos_t pos);
void cleanPosition(pos_t pos);
bool isToGrow();
pos_t chooseRandomPos(pos_t pos);
void createEntity(entity_t entity, pos_t pos);

// Function to generate a random action based on probability
bool random_action(float probability) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < probability;
}

bool random_number() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen);
}

int getRandomNumberFromVector(const std::vector<int>& numbers) {
    if (numbers.empty()) {
        // Handle case where the vector is empty
        return -1; // You might want to choose a different default value
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, numbers.size() - 1);

    return numbers[dis(gen)];
}



// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {empty, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;

int main()
{
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end(); });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
                                { 
        // Parse the JSON request body
        nlohmann::json request_body = nlohmann::json::parse(req.body);

       // Validate the request body 
        uint32_t total_entinties = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
        if (total_entinties > NUM_ROWS * NUM_ROWS) {
        res.code = 400;
        res.body = "Too many entities";
        res.end();
        return;
        }

        // Clear the entity grid
        entity_grid.clear();
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0}));
        
        // Create the entities

        //getting the numbers from the web page
        uint32_t initial_num_plants = (uint32_t)request_body["plants"];
        uint32_t initial_num_herb = (uint32_t)request_body["herbivores"];
        uint32_t initial_num_carn = (uint32_t)request_body["carnivores"];

        entity_t new_plant = {plant, 0, 0};
        entity_t new_herb = {herbivore, 100, 0};
        entity_t new_carn = {carnivore, 100, 0};
        

        initilize_entity(new_plant,initial_num_plants);
        initilize_entity(new_herb,initial_num_herb);
        initilize_entity(new_carn,initial_num_carn);
    
        // <YOUR CODE HERE>

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        res.body = json_grid.dump();
        res.end(); });


    // Endpoint to process HTTP GET requests for the next simulation iteration
    CROW_ROUTE(app, "/next-iteration")
        .methods("GET"_method)([]()
                               {
        //int init_iteration = iteration;
    pos_t current_pos;
        // Simulate the next iteration
        // Iterate over the entity grid and simulate the behaviour of each entity
        for(int i = 0;i<NUM_ROWS;i++){
            for(int j = 0;j<NUM_ROWS;j++){
                current_pos.i = i;
                current_pos.j = j;
                simulate_ent(current_pos);
            }
        }
        //printf("%d\n",iteration);
        iteration++;
        
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}




void initilize_entity (entity_t entity, int total_ent){
    int num_ent = 0;
    while(num_ent < total_ent){
        pos_t current_pos;
        for(int i = 0; i< NUM_ROWS;i ++){
            for(int j = 0; j < NUM_ROWS;j++){
                current_pos.i = i;
                current_pos.j = j;
                if(isNoEntitity(current_pos) && num_ent < total_ent){
                    if(random_action(ENTITY_CREATE_PROBABILITY) && num_ent < total_ent){
                        entity_grid[current_pos.i][current_pos.j] = entity;
                        num_ent++;
                    }
                }
            }  
        }
    }
}

bool isNoEntitity(pos_t pos){

    if(entity_grid[pos.i][pos.j].type != empty) return(false);

    else return(true);
}

void simulate_ent(pos_t pos){

    if(entity_grid[pos.i][pos.j].type == plant)
        simulate_plant(pos);

    if(entity_grid[pos.i][pos.j].type == herbivore)
        simulate_herb(pos);

    if(entity_grid[pos.i][pos.j].type == carnivore)
        simulate_carn(pos);
        
}

void simulate_plant(pos_t pos){
    //I am a plant
    if(entity_grid[pos.i][pos.j].age == PLANT_MAXIMUM_AGE){
        cleanPosition(pos);
    }
    
    entity_grid[pos.i][pos.j].age +=1;
}

void simulate_herb(pos_t pos){
    //I am a plant

    if(isToGrow()){
        pos_t new_plant_pos = chooseRandomPos(pos);
        createEntity(entity_grid[pos.i][pos.j], new_plant_pos);
    }

    if(entity_grid[pos.i][pos.j].age == HERBIVORE_MAXIMUM_AGE){
        cleanPosition(pos);
    }
    
    entity_grid[pos.i][pos.j].age +=1;
}

void simulate_carn(pos_t pos){
    //I am a plant
    if(entity_grid[pos.i][pos.j].age == CARNIVORE_MAXIMUM_AGE){
        cleanPosition(pos);
    }
    
    entity_grid[pos.i][pos.j].age +=1;
}


void cleanPosition(pos_t pos){
    entity_grid[pos.i][pos.j].type = empty;
    entity_grid[pos.i][pos.j].age = 0;
    entity_grid[pos.i][pos.j].energy = 0;
}

bool isToGrow(){
    if(random_action(PLANT_REPRODUCTION_PROBABILITY)) return(true);
    else return(false);
}

pos_t chooseRandomPos(pos_t pos){
    std::vector<int> vec_pos_i;
    std::vector<int> vec_pos_j;

    for(int i = pos.i -1; i<= pos.i+1;i++){
        for(int j = pos.j-1;j <= pos.j < pos.j+1;j++){
            if(isNoEntitity(pos)){
                vec_pos_i.push_back(i);
                vec_pos_j.push_back(j);
            }
        }
    }

    int new_i = getRandomNumberFromVector(vec_pos_i);
    int new_j = getRandomNumberFromVector(vec_pos_j);

    pos_t new_pos;
    pos.i = new_i;
    pos.j = new_j;
    
    return(new_pos);
}

void createEntity(entity_t entity, pos_t pos){
    entity_grid[pos.i][pos.j].type = entity.type;
    entity_grid[pos.i][pos.j].age = 0;
    entity_grid[pos.i][pos.j].energy = entity.energy;
    
}