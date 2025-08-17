import cv2
import mediapipe as mp
import numpy as np
import pyttsx3
import speech_recognition as sr
import threading

# ---------------------------
# Initialize MediaPipe Pose
# ---------------------------
mp_pose = mp.solutions.pose
pose = mp_pose.Pose()

# ---------------------------
# Initialize TTS engine
# ---------------------------
engine = pyttsx3.init()
engine.setProperty('rate', 160)
engine.setProperty('volume', 1.0)
voices = engine.getProperty('voices')
if len(voices) > 1:
    engine.setProperty('voice', voices[1].id)  # switch to female voice

def speak(text):
    engine.say(text)
    engine.runAndWait()

# ---------------------------
# Variables
# ---------------------------
rep_count = 0
in_rep = False
exercise_started = False
quit_flag = False
instruction_text = "Say 'start' to begin bicep curls. Say 'quit' to exit."

# ---------------------------
# Helper Functions
# ---------------------------
def calculate_angle(a, b, c):
    a = np.array(a)
    b = np.array(b)
    c = np.array(c)
    ab = b - a
    bc = c - b
    angle = np.arctan2(bc[1], bc[0]) - np.arctan2(ab[1], ab[0])
    angle = np.abs(np.degrees(angle))
    if angle > 180:
        angle = 360 - angle
    return angle

def count_reps(landmarks):
    global rep_count, in_rep
    left_shoulder = landmarks[mp_pose.PoseLandmark.LEFT_SHOULDER.value]
    left_elbow = landmarks[mp_pose.PoseLandmark.LEFT_ELBOW.value]
    left_wrist = landmarks[mp_pose.PoseLandmark.LEFT_WRIST.value]

    shoulder_coords = (left_shoulder.x, left_shoulder.y)
    elbow_coords = (left_elbow.x, left_elbow.y)
    wrist_coords = (left_wrist.x, left_wrist.y)

    angle = calculate_angle(shoulder_coords, elbow_coords, wrist_coords)

    if angle < 30:  # Flexed
        if not in_rep:
            in_rep = True
            rep_count += 1
            print(f"Rep Count: {rep_count}")
            speak(f"Rep {rep_count}")
            if rep_count % 10 == 0:
                speak("Great job! Keep going.")
    elif angle > 160:  # Extended
        in_rep = False

# ---------------------------
# Voice Command Listener
# ---------------------------
def listen_for_commands():
    global exercise_started, quit_flag
    recognizer = sr.Recognizer()
    mic = sr.Microphone()
    with mic as source:
        recognizer.adjust_for_ambient_noise(source)
        speak("Voice commands activated. Say start or quit.")

    while not quit_flag:
        try:
            with mic as source:
                audio = recognizer.listen(source, phrase_time_limit=3)
            command = recognizer.recognize_google(audio).lower()
            print("Heard:", command)

            if "start" in command and not exercise_started:
                exercise_started = True
                speak("Starting bicep curls. Keep your body in frame and curl up.")
                rep_count = 0

            elif "quit" in command:
                speak(f"Workout ended. You completed {rep_count} reps. Great job!")
                quit_flag = True
                break

        except sr.UnknownValueError:
            continue  # ignore unrecognized speech
        except sr.RequestError:
            speak("Speech recognition service error.")
            break

# Run listener in background thread
listener_thread = threading.Thread(target=listen_for_commands, daemon=True)
listener_thread.start()

# ---------------------------
# Start video capture
# ---------------------------
cap = cv2.VideoCapture(0)
speak("Welcome! Say start to begin, or quit to exit.")

while True:
    if quit_flag:
        break

    ret, frame = cap.read()
    if not ret:
        break

    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = pose.process(frame_rgb)

    if results.pose_landmarks:
        mp.solutions.drawing_utils.draw_landmarks(frame, results.pose_landmarks, mp_pose.POSE_CONNECTIONS)
        if exercise_started:
            count_reps(results.pose_landmarks.landmark)
            instruction_text = "Perform bicep curls! Say 'quit' to stop."
    else:
        instruction_text = "Body not in frame."
        if exercise_started:
            speak("I cannot see you. Please adjust your position.")

    cv2.putText(frame, instruction_text, (10, 30), cv2.FONT_HERSHEY_SIMPLEX,
                0.7, (255, 255, 255), 2)
    cv2.putText(frame, f"Rep Count: {rep_count}", (10, 70), cv2.FONT_HERSHEY_SIMPLEX,
                0.7, (255, 255, 255), 2)

    cv2.imshow('Bicep Curl Counter', frame)

    if cv2.waitKey(1) & 0xFF == 27:  # ESC fallback
        quit_flag = True
        break

cap.release()
cv2.destroyAllWindows()
