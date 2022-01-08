 /* You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.bluetooth.audio;

import android.hardware.bluetooth.audio.AptxAdaptiveChannelMode;
import android.hardware.bluetooth.audio.AptxMode;
import android.hardware.bluetooth.audio.AptXSinkBuffering;
import android.hardware.bluetooth.audio.AptxAdaptive_TTP;
import android.hardware.bluetooth.audio.InputMode;


@VintfStability
parcelable AptxAdaptiveCapabilities {
  int[] sampleRateHz;
  AptxAdaptiveChannelMode channelMode;
  byte[] bitsPerSample;
  AptxMode AptxMode;
  AptxSinkBuffering sinkBuffering;
  AptxAdaptive_TTP ttp;
  InputMode inputMode;
  int inputFadeDuration;
  byte[] aptxAdaptiveConfigStream;
}
