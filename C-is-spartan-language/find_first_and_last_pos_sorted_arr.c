/**
 * Note: The returned array must be malloced, assume caller calls free().
 */
int* searchRange(int* nums, int numsSize, int target, int* returnSize) 
{
    int* ans = malloc(sizeof(int) * 2);
    ans[0] = -1;
    ans[1] = -1;
    *returnSize = 2;

    // Edge case if numsSize == 0
    if (numsSize == 0) {
        return ans;
    }

    if (numsSize == 1) {
        if (*nums == target) {
            ans[0] = 0;
            ans[1] = 0;
            return ans;
        } else {
            return ans;
        }
    }

    int left, right;
    left = 0;
    right = numsSize - 1;

    int *right_tighter_bound = malloc(sizeof(int));
    
    *right_tighter_bound = right;

    /*
     * Find finds the min or max index for target in nums, else -1
     * Logic of setting right_tighter_bound:
     * whenever we are moving left on finding a 'bigger' mid value, we are resetting right.
     * else if (nums[mid] > target) {
            right = mid - 1;
            *right_tighter_bound = right;
        }
     * Hence the right_tighter_bound should be sufficient to pass as right in the 2nd call to find_simple 
     */
    int find(int *nums, int left, int right, int target, bool go_left_on_match, int *right_tighter_bound);


    /*
     * Does not set right_tighter_bound
     * For deduplication we can also call find with right_tighter_bound = NULL.
     * It may a good tradeoff.
     * Potential downsides:
        Hidden behavior/Magic parameters: When you see find(nums, left, right, target, false, NULL), it's not immediately clear what NULL means or what behavior it changes. You have to look at the function implementation or documentation.

        Runtime errors: If you accidentally dereference right_tighter_bound without checking for NULL first, you get a segfault. This is a classic C footgun.

        API complexity: The function now has two different modes of operation controlled by a parameter. This violates the "single responsibility principle" - the function does two different things depending on input.

        Documentation burden: You need to clearly document when to pass NULL vs a valid pointer, what happens in each case, etc.

        Testing complexity: You now need to test both code paths (NULL and non-NULL), increasing test surface area.

        After more debate with Claude: https://claude.ai/public/artifacts/1a0bbc60-2780-4dc1-809c-bf71cfa8cc4a

        I have decided to use 1 method.
     */
    // int find_simple(int *nums, int left, int right, int target, bool go_left_on_match);


    ans[0] = find(nums, left, right, target, true, right_tighter_bound);

    int optimal_left_bound = ans[0] == -1 ? left : ans[0];
    int optimal_right_bound = *right_tighter_bound == -1 ? right: *right_tighter_bound;
    ans[1] = find(nums, optimal_left_bound, optimal_right_bound, target, false, NULL /* no bound tracking */);

    return ans;
}

int find(int *nums, int left, int right, int target, bool go_left_on_match, int *right_tighter_bound)
{
    int found = -1;
    int mid;

    while (left <= right) {
        mid = left + (right - left) / 2;

        /*
         * The array's mid being < target => must go right
         * on mid element being > target => must go left
         */
        if (nums[mid] < target) {
            left = mid + 1;
        } else if (nums[mid] > target) {
            right = mid - 1;
            if (right_tighter_bound != NULL)
                *right_tighter_bound = right;
        } else {
            found = mid;
            if (go_left_on_match)
                right = mid - 1;
            else
                left = mid + 1;
        }
    }
    return found;
}

// int find_simple(int *nums, int left, int right, int target, bool go_left_on_match)
// {
//     int found = -1;
//     int mid;

//     while (left <= right) {
//         mid = left + (right - left) / 2;

//         /*
//          * The array's mid being < target => must go right
//          * on mid element being > target => must go left
//          */
//         if (nums[mid] < target) {
//             left = mid + 1;
//         } else if (nums[mid] > target) {
//             right = mid - 1;
//         } else {
//             found = mid;
//             if (go_left_on_match)
//                 right = mid - 1;
//             else
//                 left = mid + 1;
//         }
//     }
//     return found;
// }
