#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {_buffer.resize(capacity);}

long StreamReassembler::merge_lock(block_node &elm1,const block_node &elm2){
    block_node x,y;
    if (elm1.begin < elm2.begin){
        x.begin = elm1.begin;
        x.data = elm1.data;
        x.length = elm1.length;
        y.begin = elm2.begin;
        y.data = elm2.data;
        y.length = elm2.length;
    }
    else{
        x.begin = elm2.begin;
        x.data = elm2.data;
        x.length = elm2.length;
        y.begin = elm1.begin;
        y.data = elm1.data;
        y.length = elm1.length;
    }
    if (x.begin + x.length <= y.begin){
        return -1;
    }
    else if(x.begin+ x.length >= y.begin + y.length){
        elm1 = x;
        return y.length;
    }
    else{
        elm1.begin = x.begin;
        elm1.data = x.data + y.data.substr(x.begin + x.length-y.begin);
        elm1.length = elm1.data.length();
        return x.begin+x.length-y.begin;
    }
}
//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (index >= _head_index + _capacity){
        return;
    }
    block_node elem;
    if (index + data.length() <= _head_index){
        goto JUDGE_EOF;
    }
    else if(index < _head_index){
        size_t offset = _head_index-index;
        elem.begin = index+ offset;
        elem.data.assign(data.begin()+offset,data.end());
        elem.length = elem.data.length();
    }
    else{
        elem.begin = index;
        elem.data = data;
        elem.length = data.length();
    }
    _unassembled_byte += elem.length;
    do{
        if (_blocks.empty()){
            break;
        }
        long merge_bytes = 0;
        auto iter = _blocks.lower_bound(elem);
        while(iter!= _blocks.end() && (merge_bytes = merge_lock(elem,*iter)) >= 0 ){
            _unassembled_byte -= merge_bytes;
            _blocks.erase(iter);
            iter = _blocks.lower_bound(elem);
        }
        if (iter == _blocks.begin()){
            break;
        }
        iter--;
        while((merge_bytes = merge_lock(elem,*iter))>= 0){
            _unassembled_byte -= merge_bytes;
            _blocks.erase(iter);
            iter = _blocks.lower_bound(elem);
            if (iter == _blocks.begin()){
                break;
            }
            iter--;
        }
    }while(false);
    _blocks.insert(elem);

    while (!_blocks.empty() && _blocks.begin()->begin == _head_index){
        const block_node head_block = *_blocks.begin();
        size_t write_bytes = _output.write(head_block.data);
        _head_index += write_bytes;
        _unassembled_byte -= write_bytes;
        _blocks.erase(_blocks.begin());
    }
    JUDGE_EOF:
        if(eof){
            _eof_flag = true;
        }
        if(_eof_flag && empty()){
            _output.end_input();
        }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_byte; }

bool StreamReassembler::empty() const { return _unassembled_byte == 0; }
