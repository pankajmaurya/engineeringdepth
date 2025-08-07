use hound;
use pyin::{Framing, PadMode, PYINExecutor};
use std::fs::File;
use std::io::BufReader;

fn main() {
    let wav_path = "TinyBhim.wav";

    // === Load WAV file ===
    let mut reader = hound::WavReader::open(wav_path).expect("Failed to open WAV file");
    let spec = reader.spec();
    let sample_rate = spec.sample_rate;
    let samples: Vec<f64> = reader
        .samples::<i16>()
        .map(|s| s.unwrap() as f64)
        .collect();

    println!(
        "Loaded '{}' with {} samples @ {} Hz",
        wav_path,
        samples.len(),
        sample_rate
    );

    // === Normalize audio ===
    let max_val = samples
        .iter()
        .cloned()
        .fold(0.0_f64, |a, b| a.max(b.abs()));
    let normalized_samples: Vec<f64> = if max_val > 0.0 {
        samples.iter().map(|s| s / max_val).collect()
    } else {
        samples.clone()
    };

    // === Configure PYIN ===
    let fmin = 80.0;
    let fmax = 400.0;
    let frame_length = 2048;
    let hop_length = Some(512); // ~10.6ms hop at 48kHz
    let win_length = None;
    let resolution = None;

    let mut pyin_exec = PYINExecutor::new(
        fmin,
        fmax,
        sample_rate,
        frame_length,
        win_length,
        hop_length,
        resolution,
    );

    let fill_unvoiced = f64::NAN;
    let framing = Framing::Center(PadMode::Constant(0.0));

    // === Run PYIN ===
    let (timestamps, f0, voiced_flag, voiced_prob) =
        pyin_exec.pyin(&normalized_samples, fill_unvoiced, framing);

    // === Print output ===
    println!(
        "\n== PYIN Output ==\nFrames: {} | First time: {:.4}s | Last time: {:.4}s",
        timestamps.len(),
        timestamps.first().unwrap_or(&0.0),
        timestamps.last().unwrap_or(&0.0)
    );
    println!("Time(s)  | Pitch(Hz)  | Voiced | Prob");
    println!("-------------------------------------------");

    for i in 0..timestamps.len() {
        let t = timestamps[i];
        let pitch = f0[i];
        let voiced = voiced_flag[i];
        let prob = voiced_prob[i];

        if voiced {
            println!("{:<8.3} | {:<10.2} | {:<6} | {:.2}", t, pitch, voiced, prob);
        } else {
            println!("{:<8.3} | {:<10} | {:<6} | {:.2}", t, "UNVOICED", voiced, prob);
        }
    }
}

