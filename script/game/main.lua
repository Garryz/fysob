package.path = "./script/?.lua;"

function on_connect(session_id)
    print("test on connect")
end

function on_message(session_id, msg)
    print("test on message: ", msg)
end

function on_passive_clean(session_id)
    print("test passive clean session there")
end

function test_write_message(session_id, msg)
    print("test write message lua")
    write_message(session_id, msg)
end 
