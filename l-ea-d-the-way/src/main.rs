use std::{usize, ops::Shl, time::Instant, f32::{consts::PI, self}};

use esp_idf_svc::hal::{prelude::Peripherals, rmt::{TxRmtDriver, TxRmtConfig, Pulse, PinState, PulseTicks, FixedLengthSignal}, delay::FreeRtos};

fn main() {
    esp_idf_svc::sys::link_patches();
    esp_idf_svc::log::EspLogger::initialize_default();

    let peripherals = Peripherals::take()?;

    let rmt_channel = peripherals.rmt.channel0;
    let rmt_pin = peripherals.pins.gpio20;
    let rmt_config = TxRmtConfig::new().clock_divider(20); //240MHz -> 20MHz, 50ns/tick

    let tx_rmt = TxRmtDriver::new(rmt_channel, rmt_pin, &rmt_config)?;

    let mut leds: [RGB; 180];

    let time = Instant::now()/1000;


    for led_addr in 0..180 {
        leds[led_addr].set(
                ((time + led_addr )%PI).sin(),
                ((time + PI*2/3 + led_addr)%PI).sin(),
                ((time + PI*4/3 + led_addr)%PI).sin());
    }

    tx_rmt.start(led_data<180>(&leds));
    FreeRtos::delay_ms(10);

    log::info!("Hello, world!");
}

fn led_data<const S: usize>(led_colors: [RGB; S]) -> FixedLengthSignal {
    let low_signal = (Pulse::new(PinState::High, PulseTicks::new(8)?), Pulse::new(PinState::Low, PulseTicks::new(17)?));
    let high_signal = (Pulse::new(PinState::High, PulseTicks::new(17)?), Pulse::new(PinState::Low, PulseTicks::new(8)?));

    let mut signal = FixedLengthSignal::<24>::new();
    for n in 0..S{
        for b in 0..24{
            signal.set(n, if(led_colors[n].data & (1 << b)){&high_signal}else{&low_signal});
        }
    }
    signal
}

struct RGB{
    data: u32
}

impl RGB{
    fn set(&mut self,r:u8,g:u8,b:u8){
        self.data = g << 16 | r << 8 | b;
    }
}
