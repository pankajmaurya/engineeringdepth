import cv2
import mediapipe as mp
import numpy as np

# ---------------------------
# Config
# ---------------------------
VIS_THR = 0.5          # visibility threshold (0..1).
DRAW_SKELETON = True   # set False if you want only text.

# ---------------------------
# MediaPipe setup
# ---------------------------
mp_pose = mp.solutions.pose
mp_drawing = mp.solutions.drawing_utils
mp_styles = mp.solutions.drawing_styles

pose = mp_pose.Pose(
    model_complexity=1,
    smooth_landmarks=True,
    enable_segmentation=False,
    min_detection_confidence=0.5,
    min_tracking_confidence=0.5,
)

# ---------------------------
# Helpers
# ---------------------------
def lm_ok(landmarks, idx, thr=VIS_THR):
    """Landmark exists, has visibility >= thr, and is in [0,1] normalized frame."""
    lm = landmarks[idx]
    return (
        (lm.visibility if lm.visibility is not None else 0.0) >= thr
        and 0.0 <= lm.x <= 1.0
        and 0.0 <= lm.y <= 1.0
    )

def group_visible(landmarks, indices, need_at_least):
    """Return (is_visible_bool, count_visible, total)."""
    cnt = sum(lm_ok(landmarks, i) for i in indices)
    return (cnt >= need_at_least, cnt, len(indices))

def put_flag(frame, label, ok, y, extra=""):
    color = (0, 200, 0) if ok else (0, 0, 255)  # green / red
    txt = f"{label}: {'YES' if ok else 'NO'}{(' ' + extra) if extra else ''}"
    cv2.putText(frame, txt, (10, y), cv2.FONT_HERSHEY_SIMPLEX, 0.7, color, 2)

# ---------------------------
# Main loop
# ---------------------------
cap = cv2.VideoCapture(0)
prev_summary = None  # to avoid flooding the console

print("Starting… Press 'q' to quit.")

while True:
    ok, frame = cap.read()
    if not ok:
        break

    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = pose.process(frame_rgb)

    head_ok = torso_ok = hands_ok = False
    head_cnt = torso_cnt = hands_cnt = 0
    H, W = frame.shape[:2]

    if results.pose_landmarks:
        lms = results.pose_landmarks.landmark

        # Define groups
        HEAD = [
            mp_pose.PoseLandmark.NOSE.value,
            mp_pose.PoseLandmark.LEFT_EYE.value,
            mp_pose.PoseLandmark.RIGHT_EYE.value,
            mp_pose.PoseLandmark.LEFT_EAR.value,
            mp_pose.PoseLandmark.RIGHT_EAR.value,
        ]
        TORSO = [
            mp_pose.PoseLandmark.LEFT_SHOULDER.value,
            mp_pose.PoseLandmark.RIGHT_SHOULDER.value,
            mp_pose.PoseLandmark.LEFT_HIP.value,
            mp_pose.PoseLandmark.RIGHT_HIP.value,
        ]
        HANDS = [
            mp_pose.PoseLandmark.LEFT_WRIST.value,
            mp_pose.PoseLandmark.RIGHT_WRIST.value,
        ]

        head_ok, head_cnt, _ = group_visible(lms, HEAD, need_at_least=2)
        torso_ok, torso_cnt, _ = group_visible(lms, TORSO, need_at_least=3)
        # require BOTH wrists for "hands visible"
        hands_ok, hands_cnt, _ = group_visible(lms, HANDS, need_at_least=2)

        if DRAW_SKELETON:
            mp_drawing.draw_landmarks(
                frame,
                results.pose_landmarks,
                mp_pose.POSE_CONNECTIONS,
                landmark_drawing_spec=mp_styles.get_default_pose_landmarks_style(),
            )

    body_in_frame = head_ok and torso_ok and hands_ok

    # ---------------------------
    # Overlay debug on frame
    # ---------------------------
    put_flag(frame, "Head visible", head_ok, 30, extra=f"({head_cnt}/5 ≥2)")
    put_flag(frame, "Torso visible", torso_ok, 60, extra=f"({torso_cnt}/4 ≥3)")
    put_flag(frame, "Hands visible", hands_ok, 90, extra=f"({hands_cnt}/2 =2)")
    put_flag(frame, "BODY IN FRAME", body_in_frame, 130)

    cv2.imshow("Body Debug (Press 'q' to quit)", frame)

    # ---------------------------
    # Print to console only if status changed
    # ---------------------------
    summary = (head_ok, torso_ok, hands_ok, body_in_frame)
    if summary != prev_summary:
        print(
            f"Head: {head_ok} ({head_cnt}/5), "
            f"Torso: {torso_ok} ({torso_cnt}/4), "
            f"Hands: {hands_ok} ({hands_cnt}/2)  ->  BODY: {body_in_frame}"
        )
        prev_summary = summary

    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cap.release()
cv2.destroyAllWindows()
