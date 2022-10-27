typedef int RESEV_ID_INT;
typedef int RM_TYPE_INT;
typedef int STR_DATE_INT;
typedef int END_DATE_INT;
typedef int RM_ID_INT;

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <set>
#include <chrono>
//for reading json file
#include "jsoncpp/dist/json/json.h"
#include <algorithm>
#include <fstream>

using namespace std::chrono;

//ensures days are represented by 2 string digits
//converts string date to integer
int convertStrDatToInt(std::string date)
{
    //check if days are double or single digits
    //if single change from single to double
    //eg 2019-05-8 will become 2019-05-08
    if(date.size() == 9)
    {
        char temp = date[8];
        date.pop_back();
        date.push_back('0');
        date.push_back(temp);
    }
    //remove all '-' from string
    date.erase(std::remove(date.begin(), date.end(), '-'), date.end());
    //convert to inter and return
    return std::stoi(date);
}

struct Room
{
    RM_ID_INT roomID;
    RM_TYPE_INT roomType;
};
struct Order
{
    RESEV_ID_INT resevationID;
    RM_TYPE_INT roomType;
    STR_DATE_INT startDate;
    END_DATE_INT endDate;
};
struct Reservation
{
    Order *order;
    RM_ID_INT *roomID;
};



class Hotel
{
public:
    //if hotel ID is provided
    Hotel(int hotelID) : m_hotelID(hotelID){}
    //if hotel ID is not provided
    Hotel() = default;
    void setRoomTypes(const RM_TYPE_INT& roomType)
    {
        this->m_roomTypes.insert(roomType);
    }
    const bool checkRoomType(const RM_TYPE_INT& roomType)
    {
        return this->m_roomTypes.count(roomType);
    }
    void setRooms(const Room& room)
    {
        if(checkRoomType(room.roomType))
            this->m_rooms.insert(std::pair<RM_TYPE_INT ,const Room>(room.roomType, room));
        else std::cerr << "room type " << room.roomType << " does not exist\n";
    }
    std::multimap<RM_TYPE_INT ,const Room> m_rooms;
private:
    int m_hotelID;
    std::set <RM_TYPE_INT> m_roomTypes;
};

class OrderExection : public std::exception
{
public:
    OrderExection(RESEV_ID_INT * resevationID, std::string errorMessage)
    : m_resevationID(resevationID), m_errorMessage(errorMessage){}
    const int ID()const noexcept {return *(this->m_resevationID);}
    virtual const char* what() const noexcept override{return this->m_errorMessage.c_str();}
    
private:
    RESEV_ID_INT * m_resevationID;
    std::string m_errorMessage;
};



void printOrders(const std::map<RM_ID_INT, std::map<END_DATE_INT,Reservation>>& reservations, const Hotel& hotel)
{
    std::map<RM_ID_INT,std::set<RESEV_ID_INT>> roomAndReservationsInIt;
    std::multimap<RM_TYPE_INT ,const Room>::const_iterator roomsItr;
    for(roomsItr = hotel.m_rooms.begin(); roomsItr != hotel.m_rooms.end(); roomsItr++)
        roomAndReservationsInIt[roomsItr->second.roomID];
    // For accessing outer map
    std::map<RM_ID_INT, std::map<END_DATE_INT, Reservation> >::const_iterator itr;
    // For accessing inner map
    std::map<END_DATE_INT, Reservation>::const_iterator ptr;
    for (itr = reservations.begin(); itr != reservations.end(); itr++)
    {
        for (ptr = itr->second.begin(); ptr != itr->second.end();ptr++)
            roomAndReservationsInIt[itr->first].insert(ptr->second.order->resevationID);
    }
    std::map<RM_ID_INT,std::set<RESEV_ID_INT>>::iterator roomIDItr;
    for (roomIDItr = roomAndReservationsInIt.begin(); roomIDItr != roomAndReservationsInIt.end(); roomIDItr++)
    {
        std::set<RESEV_ID_INT>::iterator reservIDItr;
        std::cout << "Room ID " << roomIDItr->first << ": [";
        for(reservIDItr = roomIDItr->second.begin(); reservIDItr != roomIDItr->second.end(); )
        {
            std::cout << *reservIDItr;
            if(++reservIDItr != roomIDItr->second.end()) std::cout << ", ";
        }
        std::cout << "]\n";
    }
}


void printErrors(const std::vector<const OrderExection>& noRoomForTheseOrders)
{
    for(const OrderExection&  i : noRoomForTheseOrders)
        std::cout << "reservations ID: " << i.ID() << " " << i.what();
}



//this funtions assigns rooms to orders made
//It also creates a vector of exception object where room couldn't be assigned
void reservationOptimizer(Hotel& hotel,std::vector<Order>& orders, const int& numOfOrders)
{
    std::map<RM_ID_INT, std::map<END_DATE_INT,Reservation>> reservations;
    std::vector<const OrderExection> noRoomForTheseOrders;
    
    for(int i = 0; i < numOfOrders; i++)
    {
        bool roomAvailable = false;
        //check if order's room tpye exists in hotel
        if((hotel.checkRoomType(orders[i].roomType)) == 0)
        {
            std::string errorMessageNoRoomType{"specified room type does not exist in hotel\n"};
            OrderExection tempNoSuchRoomType(&(orders[i].resevationID),errorMessageNoRoomType);
            noRoomForTheseOrders.push_back( tempNoSuchRoomType);
            continue;
        }
        //check if there are at least one of such room type in hotel
        if((hotel.m_rooms.count(orders[i].roomType)) == 0)
        {
            std::string errorMessageNoRoomType{"type exists but no room does such type is listed\n"};
            OrderExection tempNoSuchRoomType(&(orders[i].resevationID),errorMessageNoRoomType);
            noRoomForTheseOrders.push_back( tempNoSuchRoomType);
            continue;
        }
        //Check if possilbe to assign a room.
        //First iterate values with key of orders room type
        //in multimap object property,m_rooms(rooms in hotel), of hotel class
        std::pair <std::multimap<RM_TYPE_INT ,const Room>::const_iterator, std::multimap<RM_TYPE_INT ,const Room>::const_iterator> ret;
        ret = hotel.m_rooms.equal_range(orders[i].roomType);
        for (std::multimap<RM_TYPE_INT ,const Room>::const_iterator it=ret.first; it!=ret.second; ++it)
        {
            //check if any reservations have been made in a particular room of unique room ID
            //and same room type as order
            if((reservations.count(it->second.roomID))==0) roomAvailable = true;
            else
            {
                //starting from the farthest reservation end date in room
                //iterate through already reserved orders end date
                std::map<END_DATE_INT,Reservation>::const_reverse_iterator rptr;
                rptr = (reservations.at(it->second.roomID)).rbegin();
                while(rptr!=(reservations.at(it->second.roomID)).rend())
                {
                    //check if order start date occurs after already reserved order end date
                    if((rptr->first) <=  orders[i].startDate)
                    {
                        //check if reservation is the farthest
                        if(rptr == (reservations.at(it->second.roomID)).rbegin())
                        {
                            roomAvailable = true;
                            break;
                        }
                        rptr--;
                        //check if current order fits in between the reservation
                        //rptr is pointing to and the one after it
                        if((rptr->second.order->startDate) >=  orders[i].endDate)
                        {
                            roomAvailable = true;
                            break;
                        }
                        rptr++;
                    }
                    rptr++;
                    //check if past the earliest reservation
                    if(rptr==(reservations.at(it->second.roomID)).rend())
                    {
                        rptr--;
                        //check if order can be the earliest reservation
                        if((rptr->second.order->startDate) >=  orders[i].endDate)
                        {
                            roomAvailable = true;
                            break;
                        }
                        rptr++;
                    }
                }
            }
            //assign rooom
            if(roomAvailable)
            {
                Reservation* reservation = new Reservation;
                reservation->order = &(orders[i]);
                reservation->roomID =(int *)&(it->second.roomID);
                reservations.insert(make_pair(it->second.roomID, std::map<END_DATE_INT, Reservation>()));
                reservations[it->second.roomID].insert(std::make_pair(orders[i].endDate, *reservation));
                delete reservation;
                break;
            }
        }
        if(roomAvailable == false)
        {
            std::string errorMessageNoRoomType{"| no room available for that period\n"};
            OrderExection tempNoSuchRoomType(&(orders[i].resevationID),errorMessageNoRoomType);
            noRoomForTheseOrders.push_back( tempNoSuchRoomType);
        }
    }
    printOrders(reservations,hotel);
    printErrors(noRoomForTheseOrders);
}


int main(int argc, const char * argv[])
{
    //get json file name  
    std::string jsonFileName = argv[1];
    std::ifstream myfile(jsonFileName);

    Json::Value jsonFile;
    Json::Reader reader;

    reader.parse( myfile, jsonFile);

    Hotel hotel;
    //set room types
    for(int i = 0; i < jsonFile["roomTypes"].size(); i++) 
        hotel.setRoomTypes(jsonFile["roomTypes"][i].asInt());
    //set rooms in hotel
    for(int i = 0; i < jsonFile["rooms"].size(); i++)
    {
        Room* room = new Room;
        room->roomType = jsonFile["rooms"][i]["roomType"].asInt();
        room->roomID = jsonFile["rooms"][i]["id"].asInt();
        hotel.setRooms(*room);
        delete room;
    }

    //get orders
    std::vector<Order> orders;
    for(int i = 0; i < jsonFile["reservations"].size(); i++)
    {
        Order order;
        order.resevationID = jsonFile["reservations"][i]["reservationId"].asInt();
        order.roomType = jsonFile["reservations"][i]["roomType"].asInt();
        order.startDate = convertStrDatToInt(jsonFile["reservations"][i]["startDate"].asString());
        order.endDate = convertStrDatToInt(jsonFile["reservations"][i]["endDate"].asString());
        orders.push_back(order);
    }
    auto start = high_resolution_clock::now();
    //make reservations
    reservationOptimizer(hotel, orders, orders.size());
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
     
    std::cout << "Time taken by function: "
        << duration.count() << " microseconds" << std::endl;
    
    return 0;
}
