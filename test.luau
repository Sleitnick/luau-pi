local LED_PIN = 5 -- see: https://github.com/WiringPi/WiringPi/blob/master/pins/pins.pdf
local BTN_PIN = 6

assert(pi.setup(), "setup failed")

pi.pinMode(LED_PIN, pi.OUTPUT)
pi.pinMode(BTN_PIN, pi.INPUT)

pi.onExit(function()
	pi.digitalWrite(LED_PIN, false)
end)

-- Spawn a coroutine task:
task.spawn(function()
	while true do
		local btnDown = pi.digitalRead(BTN_PIN)
		task.wait()
	end
end)

-- Top-level thread can yield too:
while true do
	pi.digitalWrite(LED_PIN, true)
	task.wait(0.2)
	pi.digitalWrite(LED_PIN, false)
	task.wait(0.5)
end
