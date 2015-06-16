local recvcount = 0

C.Listen("127.0.0.1",8010,function (s)
	print("new client",s)
	C.Bind(s,C.HttpDecoder(4096),function (s,rpk)
		print("recv packet",s,rpk:GetHeaders())
		for k,v in pairs(rpk:GetHeaders()) do
			print(k,v)
		end
		C.Close(s)
		--print("recv packet",rpk:ReadStr(),recvcount)
		--recvcount = recvcount + 1;
		--C.Send(s,C.NewWPacket(rpk))
	end)
end)

while true do
	C.Run(50)
end
