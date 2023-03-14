# makelua






for i in range(200):
    print(f"local i{i} = {i}")



exit(0)





f = open("test.lua", "w")

# N = (1 << 23) Careful not to commit this!!

N = 10
print(N)
s = "x = x + 1\n"
lua = f"""
function foo()

    local x = 0
    local y = 24

    if y then

""" + s * N + """

    end


    while y doi

    print(x)

end

foo()

"""
f.write(lua)



