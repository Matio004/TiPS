import numpy as np
import sounddevice as sd
import soundfile as sf
import matplotlib.pyplot as plt
from scipy import signal
import os
import time
from matplotlib import cm


class AudioConverter:
    def __init__(self):
        self.sample_rates = [8000, 16000, 22050, 44100, 48000, 96000]
        self.bit_depths = [8, 16, 24, 32]
        self.recording_duration = 5
        self.output_dir = "recordings"

        if not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)

    def record_audio(self, sample_rate=44100, duration=5):
        print(f"Nagrywanie dźwięku ({duration} sekund) przy częstotliwości próbkowania {sample_rate} Hz...")
        audio = sd.rec(int(duration * sample_rate), samplerate=sample_rate, channels=1, dtype='float64')
        sd.wait()
        print("Nagrywanie zakończone!")
        return audio.flatten()

    def quantize_audio(self, audio, bits):
        if bits >= 32:
            return audio.copy()

        max_val = np.max(np.abs(audio))
        if max_val > 1.0:
            audio = audio / max_val

        levels = 2 ** bits
        step = 2.0 / levels

        quantized_indices = np.round((audio + 1.0) / step)
        quantized_indices = np.clip(quantized_indices, 0, levels - 1)

        quantized = quantized_indices * step - 1.0

        return quantized

    def save_audio(self, audio, filename, sample_rate, subtype='PCM_16'):
        if subtype == 'PCM_8':
            subtype = 'PCM_16'
            print(f"Uwaga: Format PCM_8 nie jest obsługiwany. Używam PCM_16 zamiast tego dla pliku {filename}")

        try:
            sf.write(filename, audio, sample_rate, subtype=subtype)
            print(f"Zapisano plik: {filename}")
        except ValueError as e:
            print(f"Błąd zapisu pliku {filename} z subtype={subtype}: {e}")
            print("Próba zapisu z domyślnym formatem PCM_16...")
            sf.write(filename, audio, sample_rate, subtype='PCM_16')
            print(f"Zapisano plik z domyślnym formatem: {filename}")

    def calculate_snr(self, original, processed):

        if len(original) > len(processed):
            original = original[:len(processed)]
        elif len(processed) > len(original):
            processed = processed[:len(original)]

        noise = original - processed
        signal_power = np.sum(original ** 2)
        noise_power = np.sum(noise ** 2)

        if noise_power == 0:
            return float('inf')

        return 10 * np.log10(signal_power / noise_power)

    def resample_audio(self, audio, original_sr, target_sr):
        number_of_samples = round(len(audio) * float(target_sr) / original_sr)
        return signal.resample(audio, number_of_samples)

    def plot_waveform(self, audio, sample_rate, title, filename=None):
        duration = len(audio) / sample_rate
        time_axis = np.linspace(0, duration, len(audio))

        plt.figure(figsize=(10, 4))
        plt.plot(time_axis, audio)
        plt.title(title)
        plt.xlabel('Czas [s]')
        plt.ylabel('Amplituda')
        plt.grid(True)

        if filename:
            plt.savefig(filename)
        plt.close()

    def plot_spectrum(self, audio, sample_rate, title, filename=None):
        n = len(audio)
        yf = np.fft.rfft(audio)
        xf = np.fft.rfftfreq(n, 1 / sample_rate)

        plt.figure(figsize=(10, 4))
        plt.plot(xf, np.abs(yf))
        plt.title(title)
        plt.xlabel('Częstotliwość [Hz]')
        plt.ylabel('Amplituda')
        plt.grid(True)

        if filename:
            plt.savefig(filename)
        plt.close()

    def process_and_analyze(self):
        print("===== PRZETWORNIK A/C i C/A =====")
        print("\nNagrywanie referencyjnego sygnału dźwiękowego...")

        reference_sr = max(self.sample_rates)
        reference_audio = self.record_audio(sample_rate=reference_sr, duration=self.recording_duration)

        reference_filename = os.path.join(self.output_dir, f"reference_{reference_sr}Hz_32bit.wav")
        self.save_audio(reference_audio, reference_filename, reference_sr, subtype='FLOAT')

        self.plot_waveform(reference_audio, reference_sr,
                           f"Sygnał referencyjny ({reference_sr} Hz, 32 bit)",
                           os.path.join(self.output_dir, "reference_waveform.png"))

        self.plot_spectrum(reference_audio, reference_sr,
                           f"Widmo referencyjne ({reference_sr} Hz, 32 bit)",
                           os.path.join(self.output_dir, "reference_spectrum.png"))

        results = []

        for sr in self.sample_rates:
            for bits in self.bit_depths:
                print(f"\nPrzetwarzanie dla {sr} Hz, {bits} bit...")

                if bits == 8:
                    subtype = 'PCM_16'
                    print("Format PCM_8 nie jest obsługiwany, używam PCM_16")
                elif bits == 16:
                    subtype = 'PCM_16'
                elif bits == 24:
                    subtype = 'PCM_24'
                else:  # 32-bit
                    subtype = 'FLOAT'

                if sr != reference_sr:
                    resampled_audio = self.resample_audio(reference_audio, reference_sr, sr)
                else:
                    resampled_audio = reference_audio.copy()

                quantized_audio = self.quantize_audio(resampled_audio, bits)

                snr_value = self.calculate_snr(resampled_audio, quantized_audio)

                output_filename = os.path.join(self.output_dir, f"audio_{sr}Hz_{bits}bit.wav")
                self.save_audio(quantized_audio, output_filename, sr, subtype=subtype)

                self.plot_waveform(quantized_audio, sr,
                                   f"Sygnał ({sr} Hz, {bits} bit)",
                                   os.path.join(self.output_dir, f"waveform_{sr}Hz_{bits}bit.png"))

                self.plot_spectrum(quantized_audio, sr,
                                   f"Widmo ({sr} Hz, {bits} bit)",
                                   os.path.join(self.output_dir, f"spectrum_{sr}Hz_{bits}bit.png"))

                results.append({
                    'sample_rate': sr,
                    'bit_depth': bits,
                    'snr': snr_value,
                    'filename': output_filename
                })

                print(f"SNR: {snr_value:.2f} dB")

        self.save_results_to_csv(results)

        self.visualize_results(results)

        print("\nAnaliza zakończona. Wyniki zapisane w katalogu:", self.output_dir)

    def save_results_to_csv(self, results):
        csv_file = os.path.join(self.output_dir, "results.csv")
        with open(csv_file, 'w') as f:
            f.write("Sample Rate (Hz),Bit Depth,SNR (dB),Filename\n")
            for result in results:
                f.write(f"{result['sample_rate']},{result['bit_depth']},{result['snr']:.2f},{result['filename']}\n")
        print(f"Wyniki zapisane do {csv_file}")

    def visualize_results(self, results):
        sample_rates = sorted(list(set([r['sample_rate'] for r in results])))
        bit_depths = sorted(list(set([r['bit_depth'] for r in results])))

        snr_matrix = np.zeros((len(bit_depths), len(sample_rates)))

        for i, bits in enumerate(bit_depths):
            for j, sr in enumerate(sample_rates):
                for result in results:
                    if result['sample_rate'] == sr and result['bit_depth'] == bits:
                        snr_matrix[i, j] = result['snr']

        plt.figure(figsize=(12, 8))

        for i, bits in enumerate(bit_depths):
            plt.plot(sample_rates, snr_matrix[i, :], 'o-', label=f'{bits} bit')

        plt.title('SNR dla różnych parametrów próbkowania i kwantyzacji')
        plt.xlabel('Częstotliwość próbkowania [Hz]')
        plt.ylabel('SNR [dB]')
        plt.grid(True)
        plt.legend()
        plt.xscale('log')
        plt.xticks(sample_rates, [str(sr) for sr in sample_rates])

        plt.savefig(os.path.join(self.output_dir, "snr_comparison.png"))
        plt.close()

        fig = plt.figure(figsize=(12, 8))
        ax = fig.add_subplot(111, projection='3d')

        x = np.log10(sample_rates)
        y = bit_depths

        X, Y = np.meshgrid(x, y)
        Z = snr_matrix

        surf = ax.plot_surface(X, Y, Z, cmap='viridis', edgecolor='none', alpha=0.8)

        ax.set_title('SNR w zależności od częstotliwości próbkowania i rozdzielczości bitowej')
        ax.set_xlabel('Częstotliwość próbkowania [log10(Hz)]')
        ax.set_ylabel('Rozdzielczość bitowa')
        ax.set_zlabel('SNR [dB]')

        x_ticks = np.log10(sample_rates)
        ax.set_xticks(x_ticks)
        ax.set_xticklabels([str(sr) for sr in sample_rates])

        fig.colorbar(surf, ax=ax, shrink=0.5, aspect=5)

        plt.savefig(os.path.join(self.output_dir, "snr_3d.png"))
        plt.close()


def main():

    converter = AudioConverter()
    converter.process_and_analyze()


if __name__ == "__main__":
    main()