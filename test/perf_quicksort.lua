-- lua_sort_test.lua
math.randomseed(os.time())

local alloc_count = 0

function alloc_table()
    alloc_count = alloc_count + 1
    return {}
end

-- generate random array
function table_random(N, M)
    local array = alloc_table()
    for i = 1, N do
        -- array[#array+1] = math.random(1, M)
        array[#array+1] = i
    end
    return array
end


-- merging tables
function table_join(a, b)
    local c = alloc_table()
    local na = #a
    for i = 1, na do c[i] = a[i] end
    for i = 1, #b do c[i+na] = b[i] end
    return c
end

-- print table
function table_print(t)
    print("{ " .. table.concat(t, ", ") .. " }")
end

function table_minmax(t)
    local n = #t
    if n == 0 then return end
    if n == 1 then return t[1], t[1] end
    local min = t[1]
    local max = t[1]
    for i = 2, n do
        min = math.min(t[i], min)
        max = math.max(t[i], max)
    end
    return min, max
end

-- quicksort
function table_quicksort(t)
    local n = #t
    if n <= 1 then return t end
    local min, max = table_minmax(t)
    if min == max then return t end
    local mid = (min + max) / 2
    local upper = alloc_table()
    local lower = alloc_table()
    local un = 0
    local ul = 0
    for i = 1, n do
        local _t = t[i]
        if _t > mid then
            upper[un+1] = _t
            un = un + 1
        else
            lower[ul+1] = _t
            ul = ul + 1
        end
    end

    return table_join(
        table_quicksort(lower),
        table_quicksort(upper)
    )
end

local N = 1000000
local A = table_random(N, N)

collectgarbage("collect") -- run twice
collectgarbage("collect")
collectgarbage("stop") -- Fair comparison!
local before = os.clock()
local B = table_quicksort(A)
local after = os.clock()
collectgarbage("restart")

print("Time taken: " .. tostring(after - before))
print("Table allocations: ", alloc_count)
