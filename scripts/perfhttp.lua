requests = {}

function init()
  -- generate 50 requests of method POST to /tile/x/y/z where x,y,z are
  -- random ingegers, we can use wrk.format() to build the requests
  for i = 1, 500 do
    requests[i] = wrk.format("POST",
    "/tile/" .. math.random(0, 999) .. "/" .. math.random(0, 999) .. "/" .. math.random(0, 100000))
  end
end

function request()
  -- pick a random rqeuest from the table and return it
  return requests[math.random(1, 500)]
end
