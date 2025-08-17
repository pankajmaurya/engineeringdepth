import cv2
import numpy as np
import speech_recognition as sr

recognizer = sr.Recognizer()
mic = sr.Microphone()

cv2.namedWindow("Voice Commands", cv2.WINDOW_NORMAL)
current_text = "Say 'start' or 'quit'"

print("Listening... (ESC to exit)")

while True:
    try:
        with mic as source:
            recognizer.adjust_for_ambient_noise(source, duration=0.2)
            audio = recognizer.listen(source, timeout=1, phrase_time_limit=2)

        try:
            command = recognizer.recognize_google(audio).lower().strip()
            print("Heard:", command)

            if "start" in command:
                current_text = "START"
            elif "quit" in command:
                current_text = "QUIT"
            elif "biceps" in command:
                current_text = "BICEPS"
            elif "cobra" in command:
                current_text = "COBRA"
            elif "stop" in command:
                current_text = "STOP"
            elif "exit" in command:
                current_text = "EXIT"
            else:
                current_text = "Unrecognized"

        except sr.UnknownValueError:
            current_text = "..."
        except sr.RequestError as e:
            current_text = f"API Error: {e}"

    except sr.WaitTimeoutError:
        # Nothing heard within timeout → keep last message
        current_text = current_text  

    # --- Draw text big on screen ---
    frame = 255 * np.ones((400, 800, 3), dtype="uint8")
    font_scale = 3.0
    thickness = 6
    text_size = cv2.getTextSize(current_text, cv2.FONT_HERSHEY_SIMPLEX, font_scale, thickness)[0]
    text_x = (frame.shape[1] - text_size[0]) // 2
    text_y = (frame.shape[0] + text_size[1]) // 2
    cv2.putText(frame, current_text, (text_x, text_y),
                cv2.FONT_HERSHEY_SIMPLEX, font_scale, (0, 0, 0), thickness, cv2.LINE_AA)

    cv2.imshow("Voice Commands", frame)

    if cv2.waitKey(50) & 0xFF == 27:  # ESC
        break

cv2.destroyAllWindows()
