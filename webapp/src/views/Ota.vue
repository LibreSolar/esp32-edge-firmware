<template>
    <v-container>
        <v-layout text-xs-center wrap>
      <v-flex xs12 sm6 offset-sm3>
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
        success: false
    }
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
          this.disable_upload_btn = true
          var reader = new FileReader();
          if(!this.file) {
              this.status = "No file selected"
              this.alert = true
              this.uploading = !this.uploading;
              this.disable_upload_btn = false
              return;
          }
          reader.readAsArrayBuffer(this.file);
          reader.onload = () => {
            this.$ajax
            .post('api/v1/ota/upload', reader.result)
            .then(res => {
                this.status = "Image upload successfull";
                this.disable_flash_btn = false;
                this.disable_upload_btn = false
                this.uploading = !this.uploading;
                this.alert=false
                this.success=true
            })
            .catch(error => {
                this.status = "Could not upload image - Statuscode: " + error.response.status
                this.uploading = !this.uploading;
                this.disable_upload_btn = false
                this.disable_flash_btn = true
                this.alert=true
                this.success=false
            });
          }},
          flash: function() {
            this.flashing = !this.flashing;
            this.disable_flash_btn = true
            let id = this.$store.state.active_device_id
            this.$ajax
            .get('api/v1/ota/' + 'kk8a4zv')
            .then(res => {
                this.status = "Image flashed successfully";
                this.disable_flash_btn = false;
                this.disable_upload_btn = false
                this.flashing = !this.uploading;
                this.alert=false
                this.success=true
            })
            .catch(error => {
                this.status = "Could not be flashed: " + error.response.status + "-" + error.response.data
                this.flashing = !this.flashing;
                this.disable_flash_btn = false
                this.disable_upload_btn = false
                this.success=false
                this.alert=true
            });
          }
      }
}
</script>