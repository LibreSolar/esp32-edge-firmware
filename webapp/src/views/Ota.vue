<template>
   <v-container fill-height>
    <v-layout text-center align-center>
      <v-flex>
        <v-card>
        <v-card-title class="justify-center">Over-the-Air-Upgrade</v-card-title>
        <v-card-text>
            <v-row justify="center" dense>
                <v-col cols=10>
                    <div class="ma-auto">
                        Select an image from your PC to upload. Once uploaded, you can start the update process
                    </div>
                </v-col>
            </v-row>
            <v-row justify="center" dense>
                <v-col cols=10>
                    <div class="ma-auto">
                        Current Version: {{ fw_version_old }}
                    </div>
                </v-col>
            </v-row>
            <v-row justify="center">
                <v-col cols=10 >
                    <v-file-input
                        v-model="file"
                        label="File input"
                        outlined
                        dense
                        :show-size="1000"
                    ></v-file-input>
                </v-col>
            </v-row>
            <v-row justify="center" dense>
                <v-spacer></v-spacer>
                <v-col cols=2>
                    <v-btn
                    :loading="uploading"
                    :disabled="upload_disabled"
                    @click="upload()">
                    Upload</v-btn>
                </v-col>
                <v-spacer></v-spacer>
                <v-col cols=2>
                    <v-btn
                    :loading="flashing"
                    :disabled="flash_disabled"
                    @click="flash()">
                    Flash</v-btn></v-col>
                <v-spacer></v-spacer>
            </v-row>
            <v-row justify="center">
                <v-alert
                v-model="alert"
                dense
                outlined
                type="error"
                transition="scale-transition"
                dismissible
                >{{status}}</v-alert>
                <v-alert
                v-model="success"
                dense
                outlined
                type="success"
                transition="scale-transition"
                dismissible
                >{{status}}</v-alert>
            </v-row>
        </v-card-text>
        </v-card>
      </v-flex>
        </v-layout>
    </v-container>
</template>

<script>

export default {
  data () {
    return {
        uploading: false,
        flashing: false,
        disable_flash_btn: true,
        disable_upload_btn: false,
        file: null,
        status: null,
        alert: false,
        success: false,
        fw_version_old: null,
        fw_version_new: null
    }
  },
  mounted() {
      this.fetch_data("old")
  },
  computed: {
      flash_disabled() {
          return this.disable_flash_btn;
      },
      upload_disabled() {
          return this.disable_upload_btn;
      }
  },
  methods: {
      upload: function() {
          this.uploading = !this.uploading;
          this.disable("upload")
          var reader = new FileReader();
          if(!this.file) {
              this.status = "No file selected"
              this.show_alert()
              this.uploading = !this.uploading;
              this.enable("upload")
              return;
          }
          reader.readAsArrayBuffer(this.file);
          reader.onload = () => {
            this.$ajax
            .post('api/v1/ota/upload', reader.result)
            .then(res => {
                this.status = "Image upload successfull";
                this.enable("flash");
                this.enable("upload")
                this.uploading = !this.uploading;
                this.show_success()
            })
            .catch(error => {
                this.status = "Could not upload image - Statuscode: " + error.response.status
                this.uploading = !this.uploading;
                this.enable("upload")
                this.disable("flash")
                this.show_alert()
            });
          }},
          flash: function() {
            this.flashing = !this.flashing;
            this.disable("flash")
            let id = this.$store.state.active_device_id
            this.$ajax
            .get('api/v1/ota/' + id)
            .then(res => {
                this.disable("flash");
                this.enable("upload")
                this.flashing = !this.flashing;
                this.fetch_data("new")
            })
            .catch(error => {
                this.status = "Could not be flashed: " + error.response.status + "-" + error.response.data
                this.flashing = !this.flashing;
                this.enable("flash")
                this.enable("upload")
                this.show_alert()
            });
          },
          fetch_data: function(target) {
            let id = this.$store.state.active_device_id
            this.$ajax
            .get("api/v1/ts/" + id + "/info")
            .then(res => {
                if (target == "old") {
                    this.fw_version_old = res.data.FirmwareVersion;
                } else if (target == "new") {
                    this.fw_version_new = res.data.FirmwareVersion;
                    this.status = "Updated successfully from " + this.fw_version_old + " --> " + this.fw_version_new;
                    this.show_success()
                }
                return
            })
            .catch(error => {
                this.status = "Device Information could not be fetched: " + error.response.status + "-" + error.response.data
                this.show_alert()
                return
            });
          },
          enable: function(btn) {
              if(btn == "flash") {
                  this.disable_flash_btn = false
              } else if (btn == "upload") {
                  this.disable_upload_btn = false
              }
          },
          disable: function(btn) {
              if(btn == "flash") {
                  this.disable_flash_btn = true
              } else if (btn == "upload") {
                  this.disable_upload_btn = true
              }
          },
          show_alert: function() {
              this.success = false
              this.alert = true
          },
          show_success: function() {
              this.alert = false
              this.success = true
          }

      }
}
</script>
