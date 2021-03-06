#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstddef>
#include <vector>
#include <string>
#include <sstream>
#include "../Base64/base64.h"

enum MessageType {
    Request = 0, Reply = 1, Frag = 2, LastFrag = 3, Ack = 4,
};

class Header {
public:
    MessageType message_type;

    unsigned long long operation;      // RPC Operation
    unsigned long long rpc_id;         // RPC ID
    unsigned long long sequence_id;    // Sequence/Packet ID

    // 0 indicates no fragmentation
    // 1 indicates packet is part of a fragmented message
    // -1 indicates packet is the last in a sequence of fragmented packets
    int fragmented;

    Header() {}

    Header(MessageType _msg_type, unsigned long long _op, unsigned long long _rpc_id, int _fragmented) :
            message_type(_msg_type), operation(_op), rpc_id(_rpc_id), fragmented(_fragmented), sequence_id(0) {}

    Header(char *marshalled_header) {
        // Create stringstream from decoded payload
        std::stringstream tokenizer(marshalled_header);
        std::string token;

        tokenizer >> token;
        message_type = (MessageType) std::stoi(token);

        tokenizer >> token;
        operation = std::stoull(token);

        tokenizer >> token;
        rpc_id = std::stoull(token);

        tokenizer >> token;
        sequence_id = std::stoull(token);

        tokenizer >> token;
        fragmented = std::stoi(token);
    }

    ~Header() {}

    std::string str() {
        std::string headers;

        headers.append(std::to_string(int(message_type)) + "\n");
        headers.append(std::to_string(operation) + "\n");
        headers.append(std::to_string(rpc_id) + "\n");
        headers.append(std::to_string(sequence_id) + "\n");
        headers.append(std::to_string(fragmented) + "\n");

        return headers;
    }
};

class Payload {
public:
    std::string return_val;     // Holds return value of called RPC method, Should be 0 in case Message is a request and not a reply
    ssize_t parameters_size;     // Contains the number of RPC parameters
    std::vector<std::string> parameters;        // Holds all the RPC paramters
    bool fragmented;
    std::string fragmented_data;

    Payload() {}

    Payload(std::string _return_val, size_t params_size, std::vector<std::string> params, bool _fragmented) :
            return_val(_return_val), parameters_size(params_size), parameters(params), fragmented(_fragmented) {}

    Payload(char *marshalled_payload, bool _fragmented, bool header_exist = true) : fragmented(_fragmented) {

        // Create stringstream from decoded payload
        std::stringstream tokenizer(marshalled_payload);
        std::string token;

        if (header_exist)
            for (int i = 0; i < 6; i++)
                tokenizer >> token;

        if (fragmented) {
            getline(tokenizer, token, '\0');
            fragmented_data = token;
        } else {
            tokenizer >> token;
            std::string decoded_payload = base64_decode(token);
            tokenizer = std::stringstream(decoded_payload);
            tokenizer >> token;
            return_val = token;

            tokenizer >> token;
            parameters_size = (size_t) std::stoi(token);
            getline(tokenizer, token, '\n');
            if (parameters_size == 1) {
                std::string rem(tokenizer.str().substr((unsigned long) (tokenizer.tellg())));
                parameters.push_back(rem);
//                std::cout << base64_encode((const unsigned char *) rem.c_str(), (unsigned int) rem.size());
            } else
                for (int i = 0; i < parameters_size; i++) {
                    getline(tokenizer, token, '\n');
//                std::cout << token << std::endl;
                    parameters.push_back(token);
                }
        }
    }

    ~Payload() {}

    std::string str() {
        if (fragmented)
            return fragmented_data;

        std::string payload_str;
        payload_str.append(return_val + "\n");
        std::string param_string = std::to_string(parameters_size);
        payload_str.append(param_string + "\n");

        if(parameters_size != 0) {
            for (int i = 0; i < parameters_size - 1; i++)
                payload_str.append(parameters[i] + "\n");

            payload_str.append(parameters[parameters_size - 1]);
        }
        return base64_encode((const unsigned char *) payload_str.c_str(), (unsigned int) payload_str.size());
    }
};

// TODO: Remove p_message_size from constructor, calculate it based on input vector p_message
class Message {
protected:
    Header header;
    Payload payload;

public:
    Message();

    Message(MessageType msg_type, unsigned long long op, unsigned long long p_rpc_id, std::string _return_val,
            size_t p_message_size, std::vector<std::string> p_message);

    Message(Header _header, Payload _payload) : header(_header), payload(_payload) {}

    Message(Header _header, std::string _payload, bool fragmented, bool header_exists) : header(_header),
                                                                                         payload((char *) _payload.c_str(),
                                                                                                 fragmented,
                                                                                                 header_exists) {}

    explicit Message(char *marshalled_base64);      // Unmarshalling Constructor

    ~Message();

    std::string marshal();

    // Getters
    unsigned long long getOperation();

    unsigned long long getRPCId();

    unsigned long long getSeqId();

    int getFrag();

    std::string getReturnVal();

    std::vector<std::string> getParams();

    size_t getParamsSize();

    MessageType getMessageType();

    Header getHeader() { return header; }

    Payload getPayload() { return payload; }

    // Setters
    void setOperation(unsigned long long _operation);

    void setMessage(std::vector<std::string> params, size_t params_size);

    void setMessageType(MessageType message_type);

    void setParamsSize(size_t _params_size);

    void setRPCId(unsigned long long _rpc_id);

    void setSeqId(unsigned long long _seq_id);

    void setReturnVal(std::string _return_val);

    void setFrag(int _frag);

};

#endif // MESSAGE_H