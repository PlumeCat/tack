

(function()

    local a = { "hello", "lua", "table", "test" }
    for i, v in pairs(a) do
        print(i, v)
    end
    print("----")

    local t = {
        hello = "world",
        foo = "bar",
        baz = "quz",
        one = 1,
        two = 2,
        three = 3,
        four = 4,
        five = 5,
        six = 6
    }

    for k, v in pairs(t) do
        print(k, v)
        if type(v) == "number" and v % 2 == 0 then
            t[k] = nil
        end
    end

    print("-----")

    for k, v in pairs(t) do
        print(k, v)
    end

end)()


(function()



    local a = {
        "hello", "world",
        "this", "is"
    }




end)()