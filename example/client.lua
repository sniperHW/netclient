C.Connect("127.0.0.1",8010,function (s,success)
	if success then 
		C.Bind(s,C.PacketDecoder(),function (s,rpk)
			print("recv packet",rpk:ReadStr())
			C.Send(s,C.NewWPacket(rpk))
		end)
		local wpk = C.NewWPacket()
		wpk:WriteStr("hello")
		C.Send(s,wpk)
	end
end)

while true do
	C.Run(50)
	collectgarbage("collect")
end