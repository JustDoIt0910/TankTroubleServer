#include "server/Server.h"

using namespace TankTrouble;

int main() {

    TankTrouble::Server server(9999);
    server.serve();

//    muduo::net::Buffer buf;
//    MessageTemplate temp({
//        new FieldTemplate<std::string>("field1"),
//        new StructFieldTemplate<uint8_t, uint16_t>("field2", {"a", "b"}),
//        new ArrayFieldTemplate<StructField<uint8_t, uint16_t, std::string>>("field3", {"a", "b", "c"}),
//    });
//
//    {
//        Message m = temp.getMessage();
//
//        m.setField<Field<std::string>>("field1", "test");
//        m.setField<StructField<uint8_t, uint16_t>, uint8_t>("field2", "a", 10);
//        m.setField<StructField<uint8_t, uint16_t>, uint16_t>("field2", "b", 0x1234);
//
//        StructField<uint8_t, uint16_t, std::string> f1("info", {"a", "b", "c"});
//        f1.set<uint8_t>("a", 32);
//        f1.set<uint16_t>("b", 12);
//        f1.set<std::string>("c", "test1");
//
//        StructField<uint8_t, uint16_t, std::string> f2("info", {"a", "b", "c"});
//        f2.set<uint8_t>("a", 30);
//        f2.set<uint16_t>("b", 142);
//        f2.set<std::string>("c", "test2");
//
//        StructField<uint8_t, uint16_t, std::string> f3("info", {"a", "b", "c"});
//        f3.set<uint8_t>("a", 4);
//        f3.set<uint16_t>("b", 4321);
//        f3.set<std::string>("c", "test3");
//
//        StructField<uint8_t, uint16_t, std::string> f4("info", {"a", "b", "c"});
//        f4.set<uint8_t>("a", 200);
//        f4.set<uint16_t>("b", 2);
//        f4.set<std::string>("c", "test4");
//        m.addArrayElement("field3", f1);
//        m.addArrayElement("field3", f2);
//        m.addArrayElement("field3", f3);
//        m.addArrayElement("field3", f4);
//
//        printf("\n%zu\n", m.size());
//        m.toByteArray(&buf);
//    }
//
//
//    Message m1 = temp.getMessage();
//    m1.fill(&buf);
//
//    auto v1 = m1.getField<Field<std::string>>("field1").get();
//    auto v2 = m1.getField<StructField<uint8_t, uint16_t>>("field2").get<uint16_t>("b");
//    printf("%s\n", v1.c_str());
//    printf("%x\n", v2);
//    auto arr = m1.getArray<StructField<uint8_t, uint16_t, std::string>>("field3");
//    for(int i = 0; i < arr.length(); i++)
//        printf("{%x, %x, %s} ", arr.get(i).get<uint8_t>("a"),
//               arr.get(i).get<uint16_t>("b"),
//               arr.get(i).get<std::string>("c").c_str());


    //muduo::net::Buffer buf;
//    Codec codec;
//    Message message = codec.getEmptyMessage(MSG_ROOM_INFO);
//
//    {
//        StructField<uint8_t, std::string, uint8_t, uint8_t> elem("", {
//                "room_id", "room_name", "room_cap", "room_players"
//        });
//        elem.set<uint8_t>("room_id", 1);
//        elem.set<std::string>("room_name", "sxww");
//        elem.set<uint8_t>("room_cap", 3);
//        elem.set<uint8_t>("room_players", 2);
//        message.addArrayElement("room_infos", elem);
//        message.addArrayElement("room_infos", elem);
//        message.addArrayElement("room_infos", elem);
//    }
//
//    printf("\n%zu\n", message.size());
//    return 0;
}
