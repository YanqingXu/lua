#include "table_lib.hpp"
#include "../vm/state.hpp"
#include "../vm/table.hpp"
#include "../vm/value.hpp"
#include "../vm/function.hpp"
#include "../gc/core/gc_ref.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace Lua {

    void TableLib::registerModule(State* state) {
        // Create table module table
        auto tableModule = make_gc_table();
        
        // Register all table functions
        registerFunction(state, Value(tableModule), "insert", insert);
        registerFunction(state, Value(tableModule), "remove", remove);
        registerFunction(state, Value(tableModule), "concat", concat);
        registerFunction(state, Value(tableModule), "sort", sort);
        registerFunction(state, Value(tableModule), "pack", pack);
        registerFunction(state, Value(tableModule), "unpack", unpack);
        registerFunction(state, Value(tableModule), "move", move);
        registerFunction(state, Value(tableModule), "maxn", maxn);
        
        // Set the table module as global
        state->setGlobal("table", Value(tableModule));
        setLoaded(true);
    }

    Value TableLib::insert(State* state, int nargs) {
        if (nargs < 2 || nargs > 3) {
            throw std::runtime_error("table.insert: wrong number of arguments");
        }
        
        // Get table argument
        if (!state->isTable(1)) {
            throw std::runtime_error("table.insert: first argument must be a table");
        }
        auto table = state->toTable(1);
        
        if (nargs == 2) {
            // table.insert(table, value) - insert at end
            Value value = state->get(2);
            int len = getTableLength(*table);
            table->set(Value(len + 1), value);
        } else {
            // table.insert(table, pos, value) - insert at position
            if (!state->isNumber(2)) {
                throw std::runtime_error("table.insert: position must be a number");
            }
            int pos = static_cast<int>(state->toNumber(2));
            Value value = state->get(3);
            
            int len = getTableLength(*table);
            if (pos < 1 || pos > len + 1) {
                throw std::runtime_error("table.insert: position out of bounds");
            }
            
            // Shift elements to the right
            for (int i = len; i >= pos; i--) {
                Value elem = table->get(Value(i));
                table->set(Value(i + 1), elem);
            }
            
            // Insert new value
            table->set(Value(pos), value);
        }
        
        return Value(); // nil
    }

    Value TableLib::remove(State* state, int nargs) {
        if (nargs < 1 || nargs > 2) {
            throw std::runtime_error("table.remove: wrong number of arguments");
        }
        
        // Get table argument
        if (!state->isTable(1)) {
            throw std::runtime_error("table.remove: first argument must be a table");
        }
        auto table = state->toTable(1);
        
        int len = getTableLength(*table);
        if (len == 0) {
            return Value(); // nil
        }
        
        int pos;
        if (nargs == 1) {
            // table.remove(table) - remove last element
            pos = len;
        } else {
            // table.remove(table, pos) - remove at position
            if (!state->isNumber(2)) {
                throw std::runtime_error("table.remove: position must be a number");
            }
            pos = static_cast<int>(state->toNumber(2));
            
            if (pos < 1 || pos > len) {
                return Value(); // nil if out of bounds
            }
        }
        
        // Get the value to be removed
        Value removed = table->get(Value(pos));
        
        // Shift elements to the left
        for (int i = pos; i < len; i++) {
            Value elem = table->get(Value(i + 1));
            table->set(Value(i), elem);
        }
        
        // Remove the last element
        table->set(Value(len), Value()); // nil
        
        return removed;
    }

    Value TableLib::concat(State* state, int nargs) {
        if (nargs < 1 || nargs > 4) {
            throw std::runtime_error("table.concat: wrong number of arguments");
        }
        
        // Get table argument
        if (!state->isTable(1)) {
            throw std::runtime_error("table.concat: first argument must be a table");
        }
        auto table = state->toTable(1);
        
        // Get separator (default is empty string)
        Str sep = "";
        if (nargs >= 2 && state->isString(2)) {
            sep = state->toString(2);
        }
        
        // Get start and end indices
        int start = 1;
        int end = getTableLength(*table);
        
        if (nargs >= 3 && state->isNumber(3)) {
            start = static_cast<int>(state->toNumber(3));
        }
        if (nargs >= 4 && state->isNumber(4)) {
            end = static_cast<int>(state->toNumber(4));
        }
        
        if (start > end) {
            return Value(""); // empty string
        }
        
        std::ostringstream result;
        for (int i = start; i <= end; i++) {
            Value elem = table->get(Value(i));
            if (!elem.isNil()) {
                if (i > start) {
                    result << sep;
                }
                result << elem.toString();
            }
        }
        
        return Value(result.str());
    }

    Value TableLib::sort(State* state, int nargs) {
        if (nargs < 1 || nargs > 2) {
            throw std::runtime_error("table.sort: wrong number of arguments");
        }
        
        // Get table argument
        if (!state->isTable(1)) {
            throw std::runtime_error("table.sort: first argument must be a table");
        }
        auto table = state->toTable(1);
        
        int len = getTableLength(*table);
        if (len <= 1) {
            return Value(); // nil
        }
        
        // Get comparison function (optional)
        std::function<bool(const Value&, const Value&)> compare;
        if (nargs >= 2 && state->isFunction(2)) {
            auto compFunc = state->toFunction(2);
            compare = [state, compFunc](const Value& a, const Value& b) -> bool {
                // Call the comparison function
                Vec<Value> args = {a, b};
                Value result = state->call(Value(compFunc), args);
                return result.isTruthy();
            };
        } else {
            compare = defaultCompare;
        }
        
        // Perform quicksort
        quickSort(*table, 1, len, compare);
        
        return Value(); // nil
    }

    Value TableLib::pack(State* state, int nargs) {
        // Create new table
        auto table = make_gc_table();
        
        // Pack all arguments into the table
        for (int i = 1; i <= nargs; i++) {
            table->set(Value(i), state->get(i));
        }
        
        // Set 'n' field to the number of arguments
        table->set(Value("n"), Value(nargs));
        
        return Value(table);
    }

    Value TableLib::unpack(State* state, int nargs) {
        if (nargs < 1 || nargs > 3) {
            throw std::runtime_error("table.unpack: wrong number of arguments");
        }
        
        // Get table argument
        if (!state->isTable(1)) {
            throw std::runtime_error("table.unpack: first argument must be a table");
        }
        auto table = state->toTable(1);
        
        // Get start and end indices
        int start = 1;
        int end = getTableLength(*table);
        
        if (nargs >= 2 && state->isNumber(2)) {
            start = static_cast<int>(state->toNumber(2));
        }
        if (nargs >= 3 && state->isNumber(3)) {
            end = static_cast<int>(state->toNumber(3));
        }
        
        // Push all values from start to end onto the stack
        for (int i = start; i <= end; i++) {
            Value elem = table->get(Value(i));
            state->push(elem);
        }
        
        // Return the number of values pushed
        return Value(end - start + 1);
    }

    Value TableLib::move(State* state, int nargs) {
        if (nargs < 4 || nargs > 5) {
            throw std::runtime_error("table.move: wrong number of arguments");
        }
        
        // Get source table
        if (!state->isTable(1)) {
            throw std::runtime_error("table.move: first argument must be a table");
        }
        auto srcTable = state->toTable(1);
        
        // Get indices
        if (!state->isNumber(2) || !state->isNumber(3) || !state->isNumber(4)) {
            throw std::runtime_error("table.move: indices must be numbers");
        }
        
        int f = static_cast<int>(state->toNumber(2));  // from
        int e = static_cast<int>(state->toNumber(3));  // to (end)
        int t = static_cast<int>(state->toNumber(4));  // target position
        
        // Get destination table (default is source table)
        auto dstTable = srcTable;
        if (nargs >= 5 && state->isTable(5)) {
            dstTable = state->toTable(5);
        }
        
        // Move elements
        if (f <= e) {
            if (t > f) {
                // Move from right to left to avoid overwriting
                for (int i = e - f; i >= 0; i--) {
                    Value elem = srcTable->get(Value(f + i));
                    dstTable->set(Value(t + i), elem);
                }
            } else {
                // Move from left to right
                for (int i = 0; i <= e - f; i++) {
                    Value elem = srcTable->get(Value(f + i));
                    dstTable->set(Value(t + i), elem);
                }
            }
        }
        
        return Value(dstTable);
    }

    Value TableLib::maxn(State* state, int nargs) {
        if (nargs != 1) {
            throw std::runtime_error("table.maxn: wrong number of arguments");
        }
        
        // Get table argument
        if (!state->isTable(1)) {
            throw std::runtime_error("table.maxn: first argument must be a table");
        }
        auto table = state->toTable(1);
        
        // Find the maximum numeric index
        LuaNumber maxIndex = 0;
        
        // This is a simplified implementation
        // In a real implementation, we would iterate through all keys
        int len = getTableLength(*table);
        for (int i = 1; i <= len * 2; i++) { // Check beyond length for sparse arrays
            Value elem = table->get(Value(i));
            if (!elem.isNil()) {
                maxIndex = i;
            }
        }
        
        return Value(maxIndex);
    }

    // Helper functions implementation
    int TableLib::getTableLength(const Table& table) {
        return static_cast<int>(table.length());
    }

    void TableLib::quickSort(Table& table, int left, int right, const std::function<bool(const Value&, const Value&)>& compare) {
        if (left < right) {
            int pivotIndex = partition(table, left, right, compare);
            quickSort(table, left, pivotIndex - 1, compare);
            quickSort(table, pivotIndex + 1, right, compare);
        }
    }

    int TableLib::partition(Table& table, int left, int right, const std::function<bool(const Value&, const Value&)>& compare) {
        Value pivot = table.get(Value(right));
        int i = left - 1;
        
        for (int j = left; j < right; j++) {
            Value elem = table.get(Value(j));
            if (compare(elem, pivot)) {
                i++;
                Value temp = table.get(Value(i));
                table.set(Value(i), elem);
                table.set(Value(j), temp);
            }
        }
        
        Value temp = table.get(Value(i + 1));
        table.set(Value(i + 1), pivot);
        table.set(Value(right), temp);
        
        return i + 1;
    }

    bool TableLib::defaultCompare(const Value& a, const Value& b) {
        // Default comparison: a < b
        return a < b;
    }

    bool TableLib::isValidArrayIndex(const Value& index) {
        return index.isNumber() && index.asNumber() > 0 && 
               index.asNumber() == static_cast<int>(index.asNumber());
    }

    int TableLib::toArrayIndex(const Value& index) {
        if (isValidArrayIndex(index)) {
            return static_cast<int>(index.asNumber());
        }
        return -1; // Invalid index
    }

} // namespace Lua